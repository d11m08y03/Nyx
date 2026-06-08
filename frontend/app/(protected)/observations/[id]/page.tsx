"use client";

import { useEffect, useState } from "react";
import { use } from "react";
import Link from "next/link";
import { ArrowLeft, Image as ImageIcon, RefreshCw, Trash2, Upload } from "lucide-react";
import { useRef } from "react";
import { sessionApi, NyxApiError } from "@/lib/api";
import type { ObservationSession, PhotometryStatus } from "@/lib/types";
import { PlotlyChart } from "@/components/plotly-chart";
import { Button } from "@/components/ui/button";
import { Card, CardContent } from "@/components/ui/card";
import { Alert, AlertDescription } from "@/components/ui/alert";
import { Badge } from "@/components/ui/badge";
import {
  Dialog,
  DialogContent,
  DialogHeader,
  DialogTitle,
} from "@/components/ui/dialog";

export default function SessionPage({
  params,
}: {
  params: Promise<{ id: string }>;
}) {
  const { id } = use(params);
  const [session, setSession] = useState<ObservationSession | null>(null);
  const [photometryStatus, setPhotometryStatus] =
    useState<PhotometryStatus | null>(null);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState<string | null>(null);
  const [runningPhotometry, setRunningPhotometry] = useState(false);
  const [photometryDialog, setPhotometryDialog] = useState(false);
  const [targetX, setTargetX] = useState("");
  const [targetY, setTargetY] = useState("");
  const [photometryError, setPhotometryError] = useState<string | null>(null);
  const [deleteImageId, setDeleteImageId] = useState<string | null>(null);
  const [uploading, setUploading] = useState(false);
  const fileInputRef = useRef<HTMLInputElement>(null);
  const pollRef = useRef<ReturnType<typeof setTimeout> | null>(null);

  useEffect(() => {
    sessionApi
      .getById(id)
      .then(setSession)
      .catch((err) =>
        setError(
          err instanceof NyxApiError ? err.message : "Failed to load session.",
        ),
      )
      .finally(() => setLoading(false));
  }, [id]);

  useEffect(() => {
    const processing = session?.images.some(
      (img) => img.photometry_status === "processing",
    );
    if (!processing) {
      if (pollRef.current) clearTimeout(pollRef.current);
      return;
    }
    pollRef.current = setTimeout(async () => {
      try {
        const updated = await sessionApi.getById(id);
        setSession(updated);
      } catch {
        // silently ignore poll failures
      }
    }, 3000);
    return () => {
      if (pollRef.current) clearTimeout(pollRef.current);
    };
  }, [session, id]);

  async function runPhotometry() {
    const x = parseFloat(targetX);
    const y = parseFloat(targetY);
    if (isNaN(x) || isNaN(y)) {
      setPhotometryError("Enter valid pixel coordinates.");
      return;
    }
    setRunningPhotometry(true);
    setPhotometryError(null);
    try {
      const result = await sessionApi.runPhotometry(id, x, y);
      setPhotometryStatus(result);
      setPhotometryDialog(false);
      // Reload session so image.relative_flux values are fresh
      const updated = await sessionApi.getById(id);
      setSession(updated);
    } catch (err) {
      setPhotometryError(
        err instanceof NyxApiError ? err.message : "Photometry failed.",
      );
    } finally {
      setRunningPhotometry(false);
    }
  }

  async function handleFileChange(e: React.ChangeEvent<HTMLInputElement>) {
    const files = Array.from(e.target.files ?? []);
    if (files.length === 0) return;
    setUploading(true);
    setError(null);
    try {
      const created = await sessionApi.uploadImages(id, files);
      setSession((prev) =>
        prev ? { ...prev, images: [...prev.images, ...created] } : prev,
      );
    } catch (err) {
      setError(err instanceof NyxApiError ? err.message : "Upload failed.");
    } finally {
      setUploading(false);
      if (fileInputRef.current) fileInputRef.current.value = "";
    }
  }

  async function deleteImage(imageId: string) {
    try {
      await sessionApi.deleteImage(id, imageId);
      setSession((prev) =>
        prev
          ? {
              ...prev,
              images: prev.images.filter((img) => img.id !== imageId),
            }
          : prev,
      );
      setDeleteImageId(null);
    } catch (err) {
      setError(
        err instanceof NyxApiError ? err.message : "Failed to delete image.",
      );
    }
  }

  const images = session?.images ?? [];

  // Build photometry light curve from per-image relative_flux
  const photometryPoints = images.filter(
    (img) =>
      img.relative_flux != null &&
      isFinite(img.relative_flux) &&
      img.captured_at != null,
  );

  const photometryChartData =
    photometryPoints.length > 0
      ? [
          {
            x: photometryPoints.map((img) => img.captured_at!),
            y: photometryPoints.map((img) => img.relative_flux!),
            error_y:
              photometryPoints[0]?.relative_flux_error != null
                ? {
                    type: "data" as const,
                    array: photometryPoints.map(
                      (img) => img.relative_flux_error ?? 0,
                    ),
                    visible: true,
                    color: "rgba(148,163,184,0.3)",
                    thickness: 1,
                  }
                : undefined,
            mode: "markers" as const,
            type: "scatter" as const,
            name: "Relative flux",
            marker: { color: "#a78bfa", size: 4 },
          },
        ]
      : [];

  function MetaItem({ label, value }: { label: string; value?: string | null }) {
    return value ? (
      <>
        <span className="text-muted-foreground">{label}</span>
        <span>{value}</span>
      </>
    ) : null;
  }

  return (
    <div className="flex flex-col">
      <header className="border-b px-6 py-4">
        <div className="flex items-center gap-3">
          <Link
            href="/observations"
            className="text-muted-foreground transition-colors hover:text-foreground"
          >
            <ArrowLeft className="h-4 w-4" />
          </Link>
          <div className="min-w-0">
            <h1 className="truncate text-base font-semibold">
              {loading ? "Loading…" : (session?.target_id ?? id)}
            </h1>
            {session && (
              <p className="text-sm text-muted-foreground">
                {new Date(session.created_at).toLocaleDateString()}
              </p>
            )}
          </div>
        </div>
      </header>

      {error && (
        <div className="px-6 pt-4">
          <Alert variant="destructive">
            <AlertDescription>{error}</AlertDescription>
          </Alert>
        </div>
      )}

      <main className="flex-1 space-y-6 p-6">
        {session && (
          <Card>
            <CardContent className="grid grid-cols-2 gap-x-8 gap-y-1.5 py-4 text-sm sm:grid-cols-4">
              <MetaItem label="Target" value={session.target_id} />
              <MetaItem label="Telescope" value={session.telescope_id} />
              <MetaItem label="Camera" value={session.camera_id} />
              <MetaItem label="Location" value={session.location_id} />
              <MetaItem label="Filter" value={session.filter_id} />
              {session.notes && (
                <>
                  <span className="text-muted-foreground col-span-1">
                    Notes
                  </span>
                  <span className="col-span-3 text-sm">{session.notes}</span>
                </>
              )}
            </CardContent>
          </Card>
        )}

        {/* Images */}
        <section>
          <div className="mb-3 flex items-center justify-between">
            <h2 className="text-sm font-medium">
              Images
              {images.length > 0 && (
                <Badge variant="secondary" className="ml-2 text-xs">
                  {images.length}
                </Badge>
              )}
            </h2>
            <Button
              size="sm"
              variant="outline"
              disabled={uploading}
              onClick={() => fileInputRef.current?.click()}
            >
              {uploading ? (
                <>
                  <RefreshCw className="mr-1.5 h-3.5 w-3.5 animate-spin" />
                  Uploading…
                </>
              ) : (
                <>
                  <Upload className="mr-1.5 h-3.5 w-3.5" />
                  Upload images
                </>
              )}
            </Button>
            <input
              ref={fileInputRef}
              type="file"
              multiple
              accept="image/jpeg,image/png,image/tiff,.dng,.nef,.cr2,.cr3,.arw,.raf,.orf,.rw2,.pef"
              className="hidden"
              onChange={handleFileChange}
            />
          </div>
          {images.length === 0 ? (
            <Card className="border-dashed">
              <CardContent className="flex flex-col items-center gap-2 py-8">
                <ImageIcon className="h-7 w-7 text-muted-foreground/30" />
                <p className="text-sm text-muted-foreground">
                  No images uploaded yet.
                </p>
              </CardContent>
            </Card>
          ) : (
            <Card>
              <CardContent className="divide-y p-0">
                {images.map((img) => (
                  <div
                    key={img.id}
                    className="flex items-center justify-between px-4 py-2.5"
                  >
                    <div className="min-w-0">
                      <p className="truncate text-sm font-medium">
                        {img.original_filename}
                      </p>
                      <p className="text-xs text-muted-foreground">
                        {img.captured_at
                          ? new Date(img.captured_at).toLocaleString()
                          : "—"}
                        {img.exposure_time_seconds != null &&
                          ` · ${img.exposure_time_seconds}s`}
                        {img.photometry_status && (
                          <span className="ml-2">
                            <Badge variant="secondary" className="text-[10px]">
                              {img.photometry_status}
                            </Badge>
                          </span>
                        )}
                      </p>
                    </div>
                    <Button
                      variant="ghost"
                      size="sm"
                      className="ml-3 h-7 w-7 shrink-0 p-0 text-destructive hover:text-destructive"
                      onClick={() => setDeleteImageId(img.id)}
                    >
                      <Trash2 className="h-3.5 w-3.5" />
                    </Button>
                  </div>
                ))}
              </CardContent>
            </Card>
          )}
        </section>

        {/* Photometry */}
        <section>
          <div className="mb-3 flex items-center justify-between">
            <h2 className="text-sm font-medium">Local photometry</h2>
            <Button
              size="sm"
              variant="outline"
              onClick={() => setPhotometryDialog(true)}
              disabled={images.length === 0}
            >
              {photometryChartData.length > 0 ? (
                <>
                  <RefreshCw className="mr-1.5 h-3.5 w-3.5" />
                  Re-run
                </>
              ) : (
                "Run photometry"
              )}
            </Button>
          </div>
          {photometryStatus && (
            <Alert className="mb-3">
              <AlertDescription>
                {photometryStatus.message}
              </AlertDescription>
            </Alert>
          )}
          <Card>
            <CardContent className="px-2 pb-2 pt-2">
              {photometryChartData.length > 0 ? (
                <PlotlyChart
                  data={photometryChartData}
                  layout={{
                    xaxis: {
                      title: { text: "Time (UTC)", font: { size: 11 } },
                    },
                    yaxis: {
                      title: { text: "Relative flux", font: { size: 11 } },
                    },
                    height: 320,
                  }}
                  className="h-[320px] w-full"
                />
              ) : (
                <div className="flex h-40 items-center justify-center text-sm text-muted-foreground">
                  {images.length === 0
                    ? "Upload images to run photometry."
                    : "Run photometry to extract a light curve from your images."}
                </div>
              )}
            </CardContent>
          </Card>
        </section>
      </main>

      {/* Photometry dialog */}
      <Dialog open={photometryDialog} onOpenChange={setPhotometryDialog}>
        <DialogContent className="max-w-sm">
          <DialogHeader>
            <DialogTitle>Run photometry</DialogTitle>
          </DialogHeader>
          <p className="text-sm text-muted-foreground">
            Enter the pixel coordinates of your target star in the images.
          </p>
          {photometryError && (
            <Alert variant="destructive">
              <AlertDescription>{photometryError}</AlertDescription>
            </Alert>
          )}
          <div className="grid grid-cols-2 gap-3">
            <div className="space-y-1">
              <label className="text-xs font-medium">X pixel</label>
              <input
                type="number"
                value={targetX}
                onChange={(e) => setTargetX(e.target.value)}
                placeholder="512"
                className="flex h-9 w-full rounded-md border border-input bg-transparent px-3 py-1 text-sm shadow-sm focus-visible:outline-none focus-visible:ring-1 focus-visible:ring-ring"
              />
            </div>
            <div className="space-y-1">
              <label className="text-xs font-medium">Y pixel</label>
              <input
                type="number"
                value={targetY}
                onChange={(e) => setTargetY(e.target.value)}
                placeholder="512"
                className="flex h-9 w-full rounded-md border border-input bg-transparent px-3 py-1 text-sm shadow-sm focus-visible:outline-none focus-visible:ring-1 focus-visible:ring-ring"
              />
            </div>
          </div>
          <div className="flex justify-end gap-2 pt-1">
            <Button
              variant="outline"
              size="sm"
              onClick={() => setPhotometryDialog(false)}
            >
              Cancel
            </Button>
            <Button
              size="sm"
              disabled={runningPhotometry}
              onClick={runPhotometry}
            >
              {runningPhotometry ? (
                <>
                  <RefreshCw className="mr-1.5 h-3.5 w-3.5 animate-spin" />
                  Running…
                </>
              ) : (
                "Run"
              )}
            </Button>
          </div>
        </DialogContent>
      </Dialog>

      <Dialog
        open={deleteImageId != null}
        onOpenChange={(o) => {
          if (!o) setDeleteImageId(null);
        }}
      >
        <DialogContent className="max-w-sm">
          <DialogHeader>
            <DialogTitle>Delete image?</DialogTitle>
          </DialogHeader>
          <p className="text-sm text-muted-foreground">This cannot be undone.</p>
          <div className="flex justify-end gap-2 pt-1">
            <Button
              variant="outline"
              size="sm"
              onClick={() => setDeleteImageId(null)}
            >
              Cancel
            </Button>
            <Button
              variant="destructive"
              size="sm"
              onClick={() => deleteImageId && deleteImage(deleteImageId)}
            >
              Delete
            </Button>
          </div>
        </DialogContent>
      </Dialog>
    </div>
  );
}
