"use client";

import { useEffect, useState } from "react";
import { useForm } from "react-hook-form";
import { zodResolver } from "@hookform/resolvers/zod";
import { z } from "zod";
import { Pencil, Plus, Trash2, Wrench } from "lucide-react";
import {
  cameraApi,
  filterApi,
  mountApi,
  telescopeApi,
  NyxApiError,
} from "@/lib/api";
import type { Camera, Filter, Mount, Telescope } from "@/lib/types";
import { Button } from "@/components/ui/button";
import { Card, CardContent } from "@/components/ui/card";
import { Alert, AlertDescription } from "@/components/ui/alert";
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
import { Tabs, TabsContent, TabsList, TabsTrigger } from "@/components/ui/tabs";
import { Badge } from "@/components/ui/badge";

// ─── Schemas ────────────────────────────────────────────────────────────────

const telescopeSchema = z.object({
  name: z.string().min(1, "Required"),
  aperture_mm: z
    .string()
    .min(1, "Required")
    .refine((v) => parseFloat(v) > 0, "Must be positive"),
  focal_length_mm: z
    .string()
    .min(1, "Required")
    .refine((v) => parseFloat(v) > 0, "Must be positive"),
  optical_design: z.string().min(1, "Required"),
  brand: z.string().optional(),
  model: z.string().optional(),
});

const cameraSchema = z.object({
  name: z.string().min(1, "Required"),
  sensor_type: z.string().min(1, "Required"),
  brand: z.string().optional(),
  model: z.string().optional(),
  pixel_size_um: z.string().optional().refine(
    (v) => !v || parseFloat(v) > 0,
    "Must be positive",
  ),
  resolution: z.string().optional(),
});

const mountSchema = z.object({
  name: z.string().min(1, "Required"),
  mount_type: z.string().min(1, "Required"),
  brand: z.string().optional(),
  model: z.string().optional(),
  is_tracked: z.boolean().optional(),
  has_goto: z.boolean().optional(),
});

const filterSchema = z.object({
  name: z.string().min(1, "Required"),
  band: z.string().min(1, "Required"),
  brand: z.string().optional(),
  model: z.string().optional(),
  bandwidth_nm: z.string().optional().refine(
    (v) => !v || parseFloat(v) > 0,
    "Must be positive",
  ),
});

type TelescopeForm = z.infer<typeof telescopeSchema>;
type CameraForm = z.infer<typeof cameraSchema>;
type MountForm = z.infer<typeof mountSchema>;
type FilterForm = z.infer<typeof filterSchema>;

// ─── Generic delete dialog ────────────────────────────────────────────────

function DeleteDialog({
  open,
  onOpenChange,
  onConfirm,
}: {
  open: boolean;
  onOpenChange: (o: boolean) => void;
  onConfirm: () => Promise<void>;
}) {
  const [error, setError] = useState<string | null>(null);
  const [busy, setBusy] = useState(false);

  async function handle() {
    setBusy(true);
    try {
      await onConfirm();
      onOpenChange(false);
    } catch (err) {
      setError(err instanceof NyxApiError ? err.message : "Delete failed.");
    } finally {
      setBusy(false);
    }
  }

  return (
    <Dialog open={open} onOpenChange={onOpenChange}>
      <DialogContent className="max-w-sm">
        <DialogHeader>
          <DialogTitle>Delete item?</DialogTitle>
        </DialogHeader>
        <p className="text-sm text-muted-foreground">
          This cannot be undone.
        </p>
        {error && (
          <Alert variant="destructive">
            <AlertDescription>{error}</AlertDescription>
          </Alert>
        )}
        <div className="flex justify-end gap-2 pt-1">
          <Button
            variant="outline"
            size="sm"
            onClick={() => onOpenChange(false)}
          >
            Cancel
          </Button>
          <Button
            variant="destructive"
            size="sm"
            disabled={busy}
            onClick={handle}
          >
            {busy ? "Deleting…" : "Delete"}
          </Button>
        </div>
      </DialogContent>
    </Dialog>
  );
}

// ─── Telescopes tab ──────────────────────────────────────────────────────

function TelescopesTab() {
  const [items, setItems] = useState<Telescope[]>([]);
  const [open, setOpen] = useState(false);
  const [editing, setEditing] = useState<Telescope | null>(null);
  const [deleteTarget, setDeleteTarget] = useState<string | null>(null);
  const [loading, setLoading] = useState(true);

  const form = useForm<TelescopeForm>({
    resolver: zodResolver(telescopeSchema),
    defaultValues: {
      name: "",
      aperture_mm: "",
      focal_length_mm: "",
      optical_design: "",
      brand: "",
      model: "",
    },
  });

  useEffect(() => {
    telescopeApi.list().then(setItems).finally(() => setLoading(false));
  }, []);

  function openCreate() {
    setEditing(null);
    form.reset({ name: "", aperture_mm: "", focal_length_mm: "", optical_design: "", brand: "", model: "" });
    setOpen(true);
  }

  function openEdit(t: Telescope) {
    setEditing(t);
    form.reset({
      name: t.name,
      aperture_mm: String(t.aperture_mm),
      focal_length_mm: String(t.focal_length_mm),
      optical_design: t.optical_design,
      brand: t.brand ?? "",
      model: t.model ?? "",
    });
    setOpen(true);
  }

  async function onSubmit(values: TelescopeForm) {
    const payload = {
      name: values.name,
      aperture_mm: parseFloat(values.aperture_mm),
      focal_length_mm: parseFloat(values.focal_length_mm),
      optical_design: values.optical_design,
      brand: values.brand || undefined,
      model: values.model || undefined,
    };
    try {
      if (editing) {
        const updated = await telescopeApi.update(editing.id, payload);
        setItems((p) => p.map((t) => (t.id === editing.id ? updated : t)));
      } else {
        const created = await telescopeApi.create(payload);
        setItems((p) => [...p, created]);
      }
      setOpen(false);
    } catch (err) {
      form.setError("root", {
        message: err instanceof NyxApiError ? err.message : "Save failed.",
      });
    }
  }

  return (
    <>
      <div className="mb-3 flex justify-end">
        <Button size="sm" onClick={openCreate}>
          <Plus className="mr-1.5 h-3.5 w-3.5" />
          Add telescope
        </Button>
      </div>
      {loading ? (
        <div className="h-16 animate-pulse rounded-lg bg-muted" />
      ) : items.length === 0 ? (
        <EmptyState label="telescopes" onCreate={openCreate} />
      ) : (
        <Card>
          <Table>
            <TableHeader>
              <TableRow>
                <TableHead>Name</TableHead>
                <TableHead>Aperture</TableHead>
                <TableHead>Focal length</TableHead>
                <TableHead>Design</TableHead>
                <TableHead />
              </TableRow>
            </TableHeader>
            <TableBody>
              {items.map((t) => (
                <TableRow key={t.id}>
                  <TableCell className="font-medium">{t.name}</TableCell>
                  <TableCell className="text-sm text-muted-foreground">
                    {t.aperture_mm} mm
                  </TableCell>
                  <TableCell className="text-sm text-muted-foreground">
                    {t.focal_length_mm} mm
                  </TableCell>
                  <TableCell>
                    <Badge variant="secondary" className="text-xs">
                      {t.optical_design}
                    </Badge>
                  </TableCell>
                  <RowActions
                    onEdit={() => openEdit(t)}
                    onDelete={() => setDeleteTarget(t.id)}
                  />
                </TableRow>
              ))}
            </TableBody>
          </Table>
        </Card>
      )}

      <Dialog open={open} onOpenChange={setOpen}>
        <DialogContent className="max-w-sm">
          <DialogHeader>
            <DialogTitle>
              {editing ? "Edit telescope" : "Add telescope"}
            </DialogTitle>
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
              <TextField form={form} name="name" label="Name" placeholder="Celestron C8" />
              <div className="grid grid-cols-2 gap-3">
                <TextField form={form} name="aperture_mm" label="Aperture (mm)" placeholder="203" />
                <TextField form={form} name="focal_length_mm" label="Focal length (mm)" placeholder="2032" />
              </div>
              <TextField form={form} name="optical_design" label="Optical design" placeholder="SCT" />
              <div className="grid grid-cols-2 gap-3">
                <TextField form={form} name="brand" label="Brand (optional)" placeholder="Celestron" />
                <TextField form={form} name="model" label="Model (optional)" placeholder="C8" />
              </div>
              <DialogActions form={form} onCancel={() => setOpen(false)} />
            </form>
          </Form>
        </DialogContent>
      </Dialog>

      <DeleteDialog
        open={deleteTarget != null}
        onOpenChange={(o) => { if (!o) setDeleteTarget(null); }}
        onConfirm={async () => {
          await telescopeApi.remove(deleteTarget!);
          setItems((p) => p.filter((t) => t.id !== deleteTarget));
          setDeleteTarget(null);
        }}
      />
    </>
  );
}

// ─── Cameras tab ─────────────────────────────────────────────────────────

function CamerasTab() {
  const [items, setItems] = useState<Camera[]>([]);
  const [open, setOpen] = useState(false);
  const [editing, setEditing] = useState<Camera | null>(null);
  const [deleteTarget, setDeleteTarget] = useState<string | null>(null);
  const [loading, setLoading] = useState(true);

  const form = useForm<CameraForm>({
    resolver: zodResolver(cameraSchema),
    defaultValues: { name: "", sensor_type: "", brand: "", model: "", pixel_size_um: "", resolution: "" },
  });

  useEffect(() => {
    cameraApi.list().then(setItems).finally(() => setLoading(false));
  }, []);

  function openCreate() {
    setEditing(null);
    form.reset({ name: "", sensor_type: "", brand: "", model: "", pixel_size_um: "", resolution: "" });
    setOpen(true);
  }

  function openEdit(c: Camera) {
    setEditing(c);
    form.reset({
      name: c.name,
      sensor_type: c.sensor_type,
      brand: c.brand ?? "",
      model: c.model ?? "",
      pixel_size_um: c.pixel_size_um != null ? String(c.pixel_size_um) : "",
      resolution: c.resolution ?? "",
    });
    setOpen(true);
  }

  async function onSubmit(values: CameraForm) {
    const payload = {
      name: values.name,
      sensor_type: values.sensor_type,
      brand: values.brand || undefined,
      model: values.model || undefined,
      pixel_size_um: values.pixel_size_um ? parseFloat(values.pixel_size_um) : undefined,
      resolution: values.resolution || undefined,
    };
    try {
      if (editing) {
        const updated = await cameraApi.update(editing.id, payload);
        setItems((p) => p.map((c) => (c.id === editing.id ? updated : c)));
      } else {
        const created = await cameraApi.create(payload);
        setItems((p) => [...p, created]);
      }
      setOpen(false);
    } catch (err) {
      form.setError("root", {
        message: err instanceof NyxApiError ? err.message : "Save failed.",
      });
    }
  }

  return (
    <>
      <div className="mb-3 flex justify-end">
        <Button size="sm" onClick={openCreate}>
          <Plus className="mr-1.5 h-3.5 w-3.5" />
          Add camera
        </Button>
      </div>
      {loading ? (
        <div className="h-16 animate-pulse rounded-lg bg-muted" />
      ) : items.length === 0 ? (
        <EmptyState label="cameras" onCreate={openCreate} />
      ) : (
        <Card>
          <Table>
            <TableHeader>
              <TableRow>
                <TableHead>Name</TableHead>
                <TableHead>Sensor</TableHead>
                <TableHead>Pixel size</TableHead>
                <TableHead>Resolution</TableHead>
                <TableHead />
              </TableRow>
            </TableHeader>
            <TableBody>
              {items.map((c) => (
                <TableRow key={c.id}>
                  <TableCell className="font-medium">{c.name}</TableCell>
                  <TableCell>
                    <Badge variant="secondary" className="text-xs">
                      {c.sensor_type}
                    </Badge>
                  </TableCell>
                  <TableCell className="text-sm text-muted-foreground">
                    {c.pixel_size_um != null ? `${c.pixel_size_um} µm` : "—"}
                  </TableCell>
                  <TableCell className="text-sm text-muted-foreground">
                    {c.resolution ?? "—"}
                  </TableCell>
                  <RowActions
                    onEdit={() => openEdit(c)}
                    onDelete={() => setDeleteTarget(c.id)}
                  />
                </TableRow>
              ))}
            </TableBody>
          </Table>
        </Card>
      )}

      <Dialog open={open} onOpenChange={setOpen}>
        <DialogContent className="max-w-sm">
          <DialogHeader>
            <DialogTitle>{editing ? "Edit camera" : "Add camera"}</DialogTitle>
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
              <TextField form={form} name="name" label="Name" placeholder="ZWO ASI294MC Pro" />
              <TextField form={form} name="sensor_type" label="Sensor type" placeholder="CMOS" />
              <div className="grid grid-cols-2 gap-3">
                <TextField form={form} name="brand" label="Brand" placeholder="ZWO" />
                <TextField form={form} name="model" label="Model" placeholder="ASI294MC Pro" />
              </div>
              <div className="grid grid-cols-2 gap-3">
                <TextField form={form} name="pixel_size_um" label="Pixel size (µm)" placeholder="4.63" />
                <TextField form={form} name="resolution" label="Resolution" placeholder="4144×2822" />
              </div>
              <DialogActions form={form} onCancel={() => setOpen(false)} />
            </form>
          </Form>
        </DialogContent>
      </Dialog>

      <DeleteDialog
        open={deleteTarget != null}
        onOpenChange={(o) => { if (!o) setDeleteTarget(null); }}
        onConfirm={async () => {
          await cameraApi.remove(deleteTarget!);
          setItems((p) => p.filter((c) => c.id !== deleteTarget));
          setDeleteTarget(null);
        }}
      />
    </>
  );
}

// ─── Mounts tab ──────────────────────────────────────────────────────────

function MountsTab() {
  const [items, setItems] = useState<Mount[]>([]);
  const [open, setOpen] = useState(false);
  const [editing, setEditing] = useState<Mount | null>(null);
  const [deleteTarget, setDeleteTarget] = useState<string | null>(null);
  const [loading, setLoading] = useState(true);

  const form = useForm<MountForm>({
    resolver: zodResolver(mountSchema),
    defaultValues: { name: "", mount_type: "", brand: "", model: "", is_tracked: false, has_goto: false },
  });

  useEffect(() => {
    mountApi.list().then(setItems).finally(() => setLoading(false));
  }, []);

  function openCreate() {
    setEditing(null);
    form.reset({ name: "", mount_type: "", brand: "", model: "", is_tracked: false, has_goto: false });
    setOpen(true);
  }

  function openEdit(m: Mount) {
    setEditing(m);
    form.reset({
      name: m.name,
      mount_type: m.mount_type,
      brand: m.brand ?? "",
      model: m.model ?? "",
      is_tracked: m.is_tracked ?? false,
      has_goto: m.has_goto ?? false,
    });
    setOpen(true);
  }

  async function onSubmit(values: MountForm) {
    const payload = {
      name: values.name,
      mount_type: values.mount_type,
      brand: values.brand || undefined,
      model: values.model || undefined,
      is_tracked: values.is_tracked,
      has_goto: values.has_goto,
    };
    try {
      if (editing) {
        const updated = await mountApi.update(editing.id, payload);
        setItems((p) => p.map((m) => (m.id === editing.id ? updated : m)));
      } else {
        const created = await mountApi.create(payload);
        setItems((p) => [...p, created]);
      }
      setOpen(false);
    } catch (err) {
      form.setError("root", {
        message: err instanceof NyxApiError ? err.message : "Save failed.",
      });
    }
  }

  return (
    <>
      <div className="mb-3 flex justify-end">
        <Button size="sm" onClick={openCreate}>
          <Plus className="mr-1.5 h-3.5 w-3.5" />
          Add mount
        </Button>
      </div>
      {loading ? (
        <div className="h-16 animate-pulse rounded-lg bg-muted" />
      ) : items.length === 0 ? (
        <EmptyState label="mounts" onCreate={openCreate} />
      ) : (
        <Card>
          <Table>
            <TableHeader>
              <TableRow>
                <TableHead>Name</TableHead>
                <TableHead>Type</TableHead>
                <TableHead>Tracked</TableHead>
                <TableHead>GoTo</TableHead>
                <TableHead />
              </TableRow>
            </TableHeader>
            <TableBody>
              {items.map((m) => (
                <TableRow key={m.id}>
                  <TableCell className="font-medium">{m.name}</TableCell>
                  <TableCell>
                    <Badge variant="secondary" className="text-xs">
                      {m.mount_type}
                    </Badge>
                  </TableCell>
                  <TableCell className="text-sm text-muted-foreground">
                    {m.is_tracked ? "Yes" : "No"}
                  </TableCell>
                  <TableCell className="text-sm text-muted-foreground">
                    {m.has_goto ? "Yes" : "No"}
                  </TableCell>
                  <RowActions
                    onEdit={() => openEdit(m)}
                    onDelete={() => setDeleteTarget(m.id)}
                  />
                </TableRow>
              ))}
            </TableBody>
          </Table>
        </Card>
      )}

      <Dialog open={open} onOpenChange={setOpen}>
        <DialogContent className="max-w-sm">
          <DialogHeader>
            <DialogTitle>{editing ? "Edit mount" : "Add mount"}</DialogTitle>
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
              <TextField form={form} name="name" label="Name" placeholder="Sky-Watcher HEQ5" />
              <TextField form={form} name="mount_type" label="Type" placeholder="EQ" />
              <div className="grid grid-cols-2 gap-3">
                <TextField form={form} name="brand" label="Brand" placeholder="Sky-Watcher" />
                <TextField form={form} name="model" label="Model" placeholder="HEQ5 Pro" />
              </div>
              <div className="flex gap-4">
                <CheckboxField form={form} name="is_tracked" label="Tracked" />
                <CheckboxField form={form} name="has_goto" label="GoTo" />
              </div>
              <DialogActions form={form} onCancel={() => setOpen(false)} />
            </form>
          </Form>
        </DialogContent>
      </Dialog>

      <DeleteDialog
        open={deleteTarget != null}
        onOpenChange={(o) => { if (!o) setDeleteTarget(null); }}
        onConfirm={async () => {
          await mountApi.remove(deleteTarget!);
          setItems((p) => p.filter((m) => m.id !== deleteTarget));
          setDeleteTarget(null);
        }}
      />
    </>
  );
}

// ─── Filters tab ─────────────────────────────────────────────────────────

function FiltersTab() {
  const [items, setItems] = useState<Filter[]>([]);
  const [open, setOpen] = useState(false);
  const [editing, setEditing] = useState<Filter | null>(null);
  const [deleteTarget, setDeleteTarget] = useState<string | null>(null);
  const [loading, setLoading] = useState(true);

  const form = useForm<FilterForm>({
    resolver: zodResolver(filterSchema),
    defaultValues: { name: "", band: "", brand: "", model: "", bandwidth_nm: "" },
  });

  useEffect(() => {
    filterApi.list().then(setItems).finally(() => setLoading(false));
  }, []);

  function openCreate() {
    setEditing(null);
    form.reset({ name: "", band: "", brand: "", model: "", bandwidth_nm: "" });
    setOpen(true);
  }

  function openEdit(f: Filter) {
    setEditing(f);
    form.reset({
      name: f.name,
      band: f.band,
      brand: f.brand ?? "",
      model: f.model ?? "",
      bandwidth_nm: f.bandwidth_nm != null ? String(f.bandwidth_nm) : "",
    });
    setOpen(true);
  }

  async function onSubmit(values: FilterForm) {
    const payload = {
      name: values.name,
      band: values.band,
      brand: values.brand || undefined,
      model: values.model || undefined,
      bandwidth_nm: values.bandwidth_nm ? parseFloat(values.bandwidth_nm) : undefined,
    };
    try {
      if (editing) {
        const updated = await filterApi.update(editing.id, payload);
        setItems((p) => p.map((f) => (f.id === editing.id ? updated : f)));
      } else {
        const created = await filterApi.create(payload);
        setItems((p) => [...p, created]);
      }
      setOpen(false);
    } catch (err) {
      form.setError("root", {
        message: err instanceof NyxApiError ? err.message : "Save failed.",
      });
    }
  }

  return (
    <>
      <div className="mb-3 flex justify-end">
        <Button size="sm" onClick={openCreate}>
          <Plus className="mr-1.5 h-3.5 w-3.5" />
          Add filter
        </Button>
      </div>
      {loading ? (
        <div className="h-16 animate-pulse rounded-lg bg-muted" />
      ) : items.length === 0 ? (
        <EmptyState label="filters" onCreate={openCreate} />
      ) : (
        <Card>
          <Table>
            <TableHeader>
              <TableRow>
                <TableHead>Name</TableHead>
                <TableHead>Band</TableHead>
                <TableHead>Bandwidth</TableHead>
                <TableHead>Brand</TableHead>
                <TableHead />
              </TableRow>
            </TableHeader>
            <TableBody>
              {items.map((f) => (
                <TableRow key={f.id}>
                  <TableCell className="font-medium">{f.name}</TableCell>
                  <TableCell>
                    <Badge variant="secondary" className="text-xs">
                      {f.band}
                    </Badge>
                  </TableCell>
                  <TableCell className="text-sm text-muted-foreground">
                    {f.bandwidth_nm != null ? `${f.bandwidth_nm} nm` : "—"}
                  </TableCell>
                  <TableCell className="text-sm text-muted-foreground">
                    {f.brand ?? "—"}
                  </TableCell>
                  <RowActions
                    onEdit={() => openEdit(f)}
                    onDelete={() => setDeleteTarget(f.id)}
                  />
                </TableRow>
              ))}
            </TableBody>
          </Table>
        </Card>
      )}

      <Dialog open={open} onOpenChange={setOpen}>
        <DialogContent className="max-w-sm">
          <DialogHeader>
            <DialogTitle>{editing ? "Edit filter" : "Add filter"}</DialogTitle>
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
              <TextField form={form} name="name" label="Name" placeholder="Luminance" />
              <TextField form={form} name="band" label="Band" placeholder="L" />
              <div className="grid grid-cols-2 gap-3">
                <TextField form={form} name="brand" label="Brand" placeholder="Astronomik" />
                <TextField form={form} name="bandwidth_nm" label="Bandwidth (nm)" placeholder="50" />
              </div>
              <DialogActions form={form} onCancel={() => setOpen(false)} />
            </form>
          </Form>
        </DialogContent>
      </Dialog>

      <DeleteDialog
        open={deleteTarget != null}
        onOpenChange={(o) => { if (!o) setDeleteTarget(null); }}
        onConfirm={async () => {
          await filterApi.remove(deleteTarget!);
          setItems((p) => p.filter((f) => f.id !== deleteTarget));
          setDeleteTarget(null);
        }}
      />
    </>
  );
}

// ─── Shared helpers ───────────────────────────────────────────────────────

function EmptyState({
  label,
  onCreate,
}: {
  label: string;
  onCreate: () => void;
}) {
  return (
    <Card className="border-dashed">
      <CardContent className="flex flex-col items-center gap-3 py-10">
        <Wrench className="h-8 w-8 text-muted-foreground/30" />
        <p className="text-sm text-muted-foreground">No {label} yet.</p>
        <Button variant="outline" size="sm" onClick={onCreate}>
          Add {label.replace(/s$/, "")}
        </Button>
      </CardContent>
    </Card>
  );
}

function RowActions({
  onEdit,
  onDelete,
}: {
  onEdit: () => void;
  onDelete: () => void;
}) {
  return (
    <TableCell className="text-right">
      <div className="flex justify-end gap-1">
        <Button
          variant="ghost"
          size="sm"
          className="h-7 w-7 p-0"
          onClick={onEdit}
        >
          <Pencil className="h-3.5 w-3.5" />
        </Button>
        <Button
          variant="ghost"
          size="sm"
          className="h-7 w-7 p-0 text-destructive hover:text-destructive"
          onClick={onDelete}
        >
          <Trash2 className="h-3.5 w-3.5" />
        </Button>
      </div>
    </TableCell>
  );
}

function TextField({
  form,
  name,
  label,
  placeholder,
}: {
  // eslint-disable-next-line @typescript-eslint/no-explicit-any
  form: any;
  name: string;
  label: string;
  placeholder?: string;
}) {
  return (
    <FormField
      control={form.control}
      name={name}
      render={({ field }: { field: unknown }) => (
        <FormItem>
          <FormLabel>{label}</FormLabel>
          <FormControl>
            <Input placeholder={placeholder} {...(field as object)} />
          </FormControl>
          <FormMessage />
        </FormItem>
      )}
    />
  );
}

function CheckboxField({
  form,
  name,
  label,
}: {
  // eslint-disable-next-line @typescript-eslint/no-explicit-any
  form: any;
  name: string;
  label: string;
}) {
  return (
    <FormField
      control={form.control}
      name={name}
      render={({ field }: { field: { value: boolean; onChange: (v: boolean) => void } }) => (
        <FormItem className="flex items-center gap-2">
          <FormControl>
            <input
              type="checkbox"
              checked={field.value}
              onChange={(e) => field.onChange(e.target.checked)}
              className="h-4 w-4 accent-primary"
            />
          </FormControl>
          <FormLabel className="!mt-0 cursor-pointer font-normal">{label}</FormLabel>
        </FormItem>
      )}
    />
  );
}

function DialogActions({
  form,
  onCancel,
}: {
  // eslint-disable-next-line @typescript-eslint/no-explicit-any
  form: any;
  onCancel: () => void;
}) {
  return (
    <div className="flex justify-end gap-2 pt-2">
      <Button type="button" variant="outline" size="sm" onClick={onCancel}>
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
  );
}

// ─── Page ─────────────────────────────────────────────────────────────────

export default function EquipmentPage() {
  return (
    <div className="flex flex-col">
      <header className="border-b px-6 py-4">
        <h1 className="text-base font-semibold">Equipment</h1>
        <p className="text-sm text-muted-foreground">
          Telescopes, cameras, mounts, and filters
        </p>
      </header>

      <main className="flex-1 p-6">
        <Tabs defaultValue="telescopes">
          <TabsList className="mb-4">
            <TabsTrigger value="telescopes">Telescopes</TabsTrigger>
            <TabsTrigger value="cameras">Cameras</TabsTrigger>
            <TabsTrigger value="mounts">Mounts</TabsTrigger>
            <TabsTrigger value="filters">Filters</TabsTrigger>
          </TabsList>
          <TabsContent value="telescopes">
            <TelescopesTab />
          </TabsContent>
          <TabsContent value="cameras">
            <CamerasTab />
          </TabsContent>
          <TabsContent value="mounts">
            <MountsTab />
          </TabsContent>
          <TabsContent value="filters">
            <FiltersTab />
          </TabsContent>
        </Tabs>
      </main>
    </div>
  );
}
