"use client";

import { useEffect, useState } from "react";
import { use } from "react";
import Link from "next/link";
import { ArrowLeft, Download, RefreshCw } from "lucide-react";
import { targetApi, tessApi, NyxApiError } from "@/lib/api";
import type { Target, TessObservation, LightCurvePoint } from "@/lib/types";
import { PlotlyChart } from "@/components/plotly-chart";
import { Card, CardContent } from "@/components/ui/card";
import { Badge } from "@/components/ui/badge";
import { Alert, AlertDescription } from "@/components/ui/alert";

export default function TargetPage({
  params,
}: {
  params: Promise<{ id: string }>;
}) {
  const { id } = use(params);
  const [target, setTarget] = useState<Target | null>(null);
  const [tessObs, setTessObs] = useState<TessObservation[]>([]);
  const [selectedObs, setSelectedObs] = useState<TessObservation | null>(null);
  const [lightCurve, setLightCurve] = useState<LightCurvePoint[] | null>(null);
  const [fetchedIds, setFetchedIds] = useState<Set<string>>(new Set());
  const [fetching, setFetching] = useState(false);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState<string | null>(null);
  const [obsError, setObsError] = useState<string | null>(null);

  useEffect(() => {
    Promise.all([
      targetApi.getById(id),
      targetApi.listTessObservations(id),
    ])
      .then(([t, obs]) => {
        setTarget(t);
        setTessObs(obs);
        if (obs.length > 0) {
          const first = obs[0];
          setSelectedObs(first);
          return tessApi.getLightCurve(first.id).then((pts) => {
            setLightCurve(pts);
            setFetchedIds(new Set([first.id]));
          }).catch(() => {});
        }
      })
      .catch((err) =>
        setError(
          err instanceof NyxApiError ? err.message : "Failed to load target.",
        ),
      )
      .finally(() => setLoading(false));
  }, [id]);

  async function loadCurve(obs: TessObservation) {
    setSelectedObs(obs);
    setLightCurve(null);
    setObsError(null);

    if (fetchedIds.has(obs.id)) {
      try {
        const pts = await tessApi.getLightCurve(obs.id);
        setLightCurve(pts);
      } catch (err) {
        setObsError(
          err instanceof NyxApiError ? err.message : "Failed to load light curve.",
        );
      }
      return;
    }

    setFetching(true);
    try {
      if (!obs.data_uri) {
        await tessApi.discoverProducts(obs.id);
      }
      await tessApi.fetchLightCurve(obs.id);
      const pts = await tessApi.getLightCurve(obs.id);
      setLightCurve(pts);
      setFetchedIds((prev) => new Set([...prev, obs.id]));
    } catch (err) {
      setObsError(
        err instanceof NyxApiError
          ? err.message
          : "Failed to fetch light curve.",
      );
    } finally {
      setFetching(false);
    }
  }

  function medianOf(values: number[]): number {
    if (values.length === 0) return 1;
    const sorted = [...values].sort((a, b) => a - b);
    const mid = Math.floor(sorted.length / 2);
    return sorted.length % 2
      ? sorted[mid]
      : (sorted[mid - 1] + sorted[mid]) / 2;
  }

  const chartData = (() => {
    if (!lightCurve || lightCurve.length === 0) return [];

    const rawFlux = lightCurve
      .map((p) => p.pdcsap_flux ?? p.sap_flux)
      .filter((v): v is number => v != null && isFinite(v));

    if (rawFlux.length === 0) return [];

    const med = medianOf(rawFlux);
    const scale = med !== 0 ? med : 1;

    const x: number[] = [];
    const y: number[] = [];
    const err: number[] = [];
    let hasErr = false;

    for (const p of lightCurve) {
      const f = p.pdcsap_flux ?? p.sap_flux;
      if (f == null || !isFinite(f)) continue;
      x.push(p.time);
      y.push(f / scale);
      const e = p.pdcsap_flux_err;
      if (e != null && isFinite(e)) {
        err.push(e / scale);
        hasErr = true;
      } else {
        err.push(0);
      }
    }

    return [
      {
        x,
        y,
        error_y: hasErr
          ? {
              type: "data" as const,
              array: err,
              visible: true,
              color: "rgba(148,163,184,0.25)",
              thickness: 1,
            }
          : undefined,
        mode: "markers" as const,
        type: "scatter" as const,
        name: selectedObs?.obsid ?? "",
        marker: { color: "#60a5fa", size: 3, opacity: 0.8 },
      },
    ];
  })();

  function formatTimeRange(start: number, end: number) {
    return `${start.toFixed(1)} – ${end.toFixed(1)} BTJD`;
  }

  return (
    <div className="flex flex-col">
      <header className="border-b px-6 py-4">
        <div className="flex items-center gap-3">
          <Link
            href="/catalog"
            className="text-muted-foreground transition-colors hover:text-foreground"
          >
            <ArrowLeft className="h-4 w-4" />
          </Link>
          <div className="min-w-0">
            <h1 className="truncate text-base font-semibold">
              {loading ? "Loading…" : (target?.canonical_name ?? id)}
            </h1>
            {target?.target_type && (
              <p className="text-sm text-muted-foreground">
                {target.target_type}
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
        {/* Target metadata */}
        {target && (
          <Card>
            <CardContent className="grid grid-cols-2 gap-x-8 gap-y-1.5 py-4 text-sm sm:grid-cols-4">
              {target.right_ascension != null && (
                <>
                  <span className="text-muted-foreground">RA</span>
                  <span className="font-mono">
                    {target.right_ascension.toFixed(6)}°
                  </span>
                </>
              )}
              {target.declination != null && (
                <>
                  <span className="text-muted-foreground">Dec</span>
                  <span className="font-mono">
                    {target.declination.toFixed(6)}°
                  </span>
                </>
              )}
              {tessObs.length > 0 && (
                <>
                  <span className="text-muted-foreground">
                    TESS observations
                  </span>
                  <span>{tessObs.length}</span>
                </>
              )}
            </CardContent>
          </Card>
        )}

        {/* TESS observation selector */}
        {tessObs.length > 0 && (
          <section>
            <h2 className="mb-3 text-sm font-medium">TESS observations</h2>
            <div className="flex flex-wrap gap-2">
              {tessObs.map((obs) => (
                <button
                  key={obs.id}
                  onClick={() => loadCurve(obs)}
                  disabled={fetching}
                  className={`flex items-center gap-1.5 rounded-md border px-3 py-1.5 text-xs transition-colors ${
                    selectedObs?.id === obs.id
                      ? "border-primary bg-primary/10 text-primary"
                      : "border-border text-muted-foreground hover:border-foreground/30 hover:text-foreground"
                  }`}
                >
                  {obs.cadence_seconds}s cadence
                  <span className="text-muted-foreground/60">
                    {formatTimeRange(obs.start_time, obs.end_time)}
                  </span>
                  {fetchedIds.has(obs.id) ? (
                    <Badge
                      variant="secondary"
                      className="h-4 px-1 text-[9px] leading-none"
                    >
                      ready
                    </Badge>
                  ) : (
                    <Download className="h-3 w-3" />
                  )}
                </button>
              ))}
            </div>
          </section>
        )}

        {/* Light curve chart */}
        <section>
          <div className="mb-3 flex items-center justify-between">
            <h2 className="text-sm font-medium">
              {selectedObs
                ? `Light curve — ${selectedObs.obsid}`
                : "TESS light curve"}
            </h2>
            {fetching && (
              <div className="flex items-center gap-1.5 text-xs text-muted-foreground">
                <RefreshCw className="h-3 w-3 animate-spin" />
                Fetching from MAST…
              </div>
            )}
          </div>
          <Card>
            <CardContent className="px-2 pb-2 pt-2">
              {lightCurve && lightCurve.length > 0 ? (
                <PlotlyChart
                  data={chartData}
                  layout={{
                    xaxis: {
                      title: { text: "Time (BTJD)", font: { size: 11 } },
                    },
                    yaxis: {
                      title: { text: "Normalised flux", font: { size: 11 } },
                    },
                    height: 380,
                  }}
                  className="h-[380px] w-full"
                />
              ) : fetching ? (
                <div className="flex h-64 items-center justify-center text-sm text-muted-foreground">
                  <RefreshCw className="mr-2 h-4 w-4 animate-spin" />
                  Downloading light curve from MAST…
                </div>
              ) : obsError ? (
                <div className="flex h-64 flex-col items-center justify-center gap-2 px-6 text-center">
                  <p className="text-sm text-destructive">{obsError}</p>
                  <p className="text-xs text-muted-foreground">
                    This observation may not have a light curve product. Try
                    selecting a different one.
                  </p>
                </div>
              ) : tessObs.length === 0 ? (
                <div className="flex h-64 items-center justify-center text-sm text-muted-foreground">
                  No TESS observations found for this target.
                </div>
              ) : (
                <div className="flex h-64 items-center justify-center text-sm text-muted-foreground">
                  Select an observation above to load its light curve.
                </div>
              )}
            </CardContent>
          </Card>
        </section>
      </main>
    </div>
  );
}
