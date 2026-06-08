"use client";

import { useEffect, useState } from "react";
import { useForm } from "react-hook-form";
import { zodResolver } from "@hookform/resolvers/zod";
import { z } from "zod";
import { MapPin, Pencil, Plus, Trash2 } from "lucide-react";
import { locationApi, NyxApiError } from "@/lib/api";
import type { ObservingLocation } from "@/lib/types";
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
import {
  Form,
  FormControl,
  FormField,
  FormItem,
  FormLabel,
  FormMessage,
} from "@/components/ui/form";
import { Input } from "@/components/ui/input";
import {
  Table,
  TableBody,
  TableCell,
  TableHead,
  TableHeader,
  TableRow,
} from "@/components/ui/table";

const locationSchema = z.object({
  name: z.string().min(1, "Name required"),
  latitude: z
    .string()
    .min(1, "Required")
    .refine((v) => !isNaN(parseFloat(v)) && parseFloat(v) >= -90 && parseFloat(v) <= 90, {
      message: "Must be -90 to 90",
    }),
  longitude: z
    .string()
    .min(1, "Required")
    .refine(
      (v) => !isNaN(parseFloat(v)) && parseFloat(v) >= -180 && parseFloat(v) <= 180,
      { message: "Must be -180 to 180" },
    ),
  bortle_class: z
    .string()
    .optional()
    .refine((v) => !v || (!isNaN(parseInt(v)) && parseInt(v) >= 1 && parseInt(v) <= 9), {
      message: "Must be 1–9",
    }),
});

type FormValues = z.infer<typeof locationSchema>;

export default function LocationsPage() {
  const [locations, setLocations] = useState<ObservingLocation[]>([]);
  const [open, setOpen] = useState(false);
  const [editing, setEditing] = useState<ObservingLocation | null>(null);
  const [deleteId, setDeleteId] = useState<string | null>(null);
  const [loading, setLoading] = useState(true);
  const [pageError, setPageError] = useState<string | null>(null);
  const [deleteError, setDeleteError] = useState<string | null>(null);

  const form = useForm<FormValues>({
    resolver: zodResolver(locationSchema),
    defaultValues: {
      name: "",
      latitude: "",
      longitude: "",
      bortle_class: "",
    },
  });

  useEffect(() => {
    load();
  }, []);

  function load() {
    locationApi
      .list()
      .then(setLocations)
      .catch((err) =>
        setPageError(err instanceof NyxApiError ? err.message : "Failed to load locations."),
      )
      .finally(() => setLoading(false));
  }

  function openCreate() {
    setEditing(null);
    form.reset({ name: "", latitude: "", longitude: "", bortle_class: "" });
    setOpen(true);
  }

  function openEdit(loc: ObservingLocation) {
    setEditing(loc);
    form.reset({
      name: loc.name,
      latitude: String(loc.latitude),
      longitude: String(loc.longitude),
      bortle_class: loc.bortle_class != null ? String(loc.bortle_class) : "",
    });
    setOpen(true);
  }

  async function onSubmit(values: FormValues) {
    const payload = {
      name: values.name,
      latitude: parseFloat(values.latitude),
      longitude: parseFloat(values.longitude),
      bortle_class: values.bortle_class ? parseInt(values.bortle_class) : undefined,
    };
    try {
      if (editing) {
        const updated = await locationApi.update(editing.id, payload);
        setLocations((prev) => prev.map((l) => (l.id === editing.id ? updated : l)));
      } else {
        const created = await locationApi.create(payload);
        setLocations((prev) => [...prev, created]);
      }
      setOpen(false);
    } catch (err) {
      form.setError("root", {
        message: err instanceof NyxApiError ? err.message : "Save failed.",
      });
    }
  }

  async function confirmDelete() {
    if (!deleteId) return;
    try {
      await locationApi.remove(deleteId);
      setLocations((prev) => prev.filter((l) => l.id !== deleteId));
      setDeleteId(null);
    } catch (err) {
      setDeleteError(err instanceof NyxApiError ? err.message : "Delete failed.");
    }
  }

  return (
    <div className="flex flex-col">
      <header className="border-b px-6 py-4">
        <div className="flex items-center justify-between">
          <div>
            <h1 className="text-base font-semibold">Locations</h1>
            <p className="text-sm text-muted-foreground">
              Observing sites and their sky conditions
            </p>
          </div>
          <Button size="sm" onClick={openCreate}>
            <Plus className="mr-1.5 h-3.5 w-3.5" />
            Add location
          </Button>
        </div>
      </header>

      <main className="flex-1 p-6">
        {pageError && (
          <Alert variant="destructive" className="mb-4">
            <AlertDescription>{pageError}</AlertDescription>
          </Alert>
        )}

        {loading ? (
          <div className="space-y-2">
            {[1, 2].map((i) => (
              <div key={i} className="h-12 animate-pulse rounded-lg bg-muted" />
            ))}
          </div>
        ) : locations.length === 0 ? (
          <Card className="border-dashed">
            <CardContent className="flex flex-col items-center gap-3 py-12">
              <MapPin className="h-8 w-8 text-muted-foreground/30" />
              <p className="text-sm text-muted-foreground">
                No locations added yet.
              </p>
              <Button variant="outline" size="sm" onClick={openCreate}>
                Add your first location
              </Button>
            </CardContent>
          </Card>
        ) : (
          <Card>
            <Table>
              <TableHeader>
                <TableRow>
                  <TableHead>Name</TableHead>
                  <TableHead>Latitude</TableHead>
                  <TableHead>Longitude</TableHead>
                  <TableHead>Bortle class</TableHead>
                  <TableHead />
                </TableRow>
              </TableHeader>
              <TableBody>
                {locations.map((loc) => (
                  <TableRow key={loc.id}>
                    <TableCell className="font-medium">{loc.name}</TableCell>
                    <TableCell className="font-mono text-sm text-muted-foreground">
                      {loc.latitude.toFixed(4)}°
                    </TableCell>
                    <TableCell className="font-mono text-sm text-muted-foreground">
                      {loc.longitude.toFixed(4)}°
                    </TableCell>
                    <TableCell>
                      {loc.bortle_class != null ? (
                        <Badge variant="secondary">Class {loc.bortle_class}</Badge>
                      ) : (
                        <span className="text-muted-foreground text-sm">—</span>
                      )}
                    </TableCell>
                    <TableCell className="text-right">
                      <div className="flex justify-end gap-1">
                        <Button
                          variant="ghost"
                          size="sm"
                          className="h-7 w-7 p-0"
                          onClick={() => openEdit(loc)}
                        >
                          <Pencil className="h-3.5 w-3.5" />
                        </Button>
                        <Button
                          variant="ghost"
                          size="sm"
                          className="h-7 w-7 p-0 text-destructive hover:text-destructive"
                          onClick={() => {
                            setDeleteError(null);
                            setDeleteId(loc.id);
                          }}
                        >
                          <Trash2 className="h-3.5 w-3.5" />
                        </Button>
                      </div>
                    </TableCell>
                  </TableRow>
                ))}
              </TableBody>
            </Table>
          </Card>
        )}
      </main>

      {/* Create/edit dialog */}
      <Dialog open={open} onOpenChange={setOpen}>
        <DialogContent className="max-w-sm">
          <DialogHeader>
            <DialogTitle>{editing ? "Edit location" : "Add location"}</DialogTitle>
          </DialogHeader>
          <Form {...form}>
            <form
              onSubmit={form.handleSubmit(onSubmit)}
              className="space-y-3 pt-1"
            >
              {form.formState.errors.root && (
                <Alert variant="destructive">
                  <AlertDescription>
                    {form.formState.errors.root.message}
                  </AlertDescription>
                </Alert>
              )}
              <FormField
                control={form.control}
                name="name"
                render={({ field }) => (
                  <FormItem>
                    <FormLabel>Name</FormLabel>
                    <FormControl>
                      <Input placeholder="Mauritius Observatory" {...field} />
                    </FormControl>
                    <FormMessage />
                  </FormItem>
                )}
              />
              <div className="grid grid-cols-2 gap-3">
                <FormField
                  control={form.control}
                  name="latitude"
                  render={({ field }) => (
                    <FormItem>
                      <FormLabel>Latitude</FormLabel>
                      <FormControl>
                        <Input placeholder="-20.2" {...field} />
                      </FormControl>
                      <FormMessage />
                    </FormItem>
                  )}
                />
                <FormField
                  control={form.control}
                  name="longitude"
                  render={({ field }) => (
                    <FormItem>
                      <FormLabel>Longitude</FormLabel>
                      <FormControl>
                        <Input placeholder="57.5" {...field} />
                      </FormControl>
                      <FormMessage />
                    </FormItem>
                  )}
                />
              </div>
              <FormField
                control={form.control}
                name="bortle_class"
                render={({ field }) => (
                  <FormItem>
                    <FormLabel>
                      Bortle class{" "}
                      <span className="text-muted-foreground">(1–9, optional)</span>
                    </FormLabel>
                    <FormControl>
                      <Input placeholder="5" {...field} />
                    </FormControl>
                    <FormMessage />
                  </FormItem>
                )}
              />
              <div className="flex justify-end gap-2 pt-2">
                <Button
                  type="button"
                  variant="outline"
                  size="sm"
                  onClick={() => setOpen(false)}
                >
                  Cancel
                </Button>
                <Button
                  type="submit"
                  size="sm"
                  disabled={form.formState.isSubmitting}
                >
                  {form.formState.isSubmitting ? "Saving…" : "Save"}
                </Button>
              </div>
            </form>
          </Form>
        </DialogContent>
      </Dialog>

      {/* Delete confirm dialog */}
      <Dialog
        open={deleteId != null}
        onOpenChange={(o) => {
          if (!o) setDeleteId(null);
        }}
      >
        <DialogContent className="max-w-sm">
          <DialogHeader>
            <DialogTitle>Delete location?</DialogTitle>
          </DialogHeader>
          <p className="text-sm text-muted-foreground">
            This will permanently remove the location and cannot be undone.
          </p>
          {deleteError && (
            <Alert variant="destructive">
              <AlertDescription>{deleteError}</AlertDescription>
            </Alert>
          )}
          <div className="flex justify-end gap-2 pt-2">
            <Button
              variant="outline"
              size="sm"
              onClick={() => setDeleteId(null)}
            >
              Cancel
            </Button>
            <Button variant="destructive" size="sm" onClick={confirmDelete}>
              Delete
            </Button>
          </div>
        </DialogContent>
      </Dialog>
    </div>
  );
}
