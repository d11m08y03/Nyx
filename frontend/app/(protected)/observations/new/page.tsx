"use client";

import { useEffect, useState } from "react";
import { use } from "react";
import { useRouter, useSearchParams } from "next/navigation";
import { useForm } from "react-hook-form";
import { zodResolver } from "@hookform/resolvers/zod";
import { z } from "zod";
import { ArrowLeft } from "lucide-react";
import Link from "next/link";
import {
  sessionApi,
  telescopeApi,
  cameraApi,
  mountApi,
  filterApi,
  locationApi,
  NyxApiError,
} from "@/lib/api";
import type {
  Camera,
  Filter,
  Mount,
  ObservingLocation,
  Telescope,
} from "@/lib/types";
import { Button } from "@/components/ui/button";
import { Card, CardContent, CardHeader, CardTitle } from "@/components/ui/card";
import { Alert, AlertDescription } from "@/components/ui/alert";
import {
  Form,
  FormControl,
  FormField,
  FormItem,
  FormLabel,
  FormMessage,
} from "@/components/ui/form";
import { Input } from "@/components/ui/input";
import { Textarea } from "@/components/ui/textarea";

const schema = z.object({
  target_name: z.string().min(1, "Required — resolve a target first"),
  target_id: z.string().min(1, "Required"),
  telescope_id: z.string().min(1, "Select a telescope"),
  camera_id: z.string().min(1, "Select a camera"),
  mount_id: z.string().min(1, "Select a mount"),
  location_id: z.string().min(1, "Select a location"),
  filter_id: z.string().optional(),
  notes: z.string().optional(),
});

type FormValues = z.infer<typeof schema>;

export default function NewSessionPage() {
  const router = useRouter();
  const searchParams = useSearchParams();

  const [telescopes, setTelescopes] = useState<Telescope[]>([]);
  const [cameras, setCameras] = useState<Camera[]>([]);
  const [mounts, setMounts] = useState<Mount[]>([]);
  const [filters, setFilters] = useState<Filter[]>([]);
  const [locations, setLocations] = useState<ObservingLocation[]>([]);
  const [loadingEquip, setLoadingEquip] = useState(true);

  const prefilledTargetId = searchParams.get("target_id") ?? "";
  const prefilledTargetName = searchParams.get("target_name") ?? "";

  const form = useForm<FormValues>({
    resolver: zodResolver(schema),
    defaultValues: {
      target_name: prefilledTargetName,
      target_id: prefilledTargetId,
      telescope_id: "",
      camera_id: "",
      mount_id: "",
      location_id: "",
      filter_id: "",
      notes: "",
    },
  });

  useEffect(() => {
    Promise.all([
      telescopeApi.list(),
      cameraApi.list(),
      mountApi.list(),
      filterApi.list(),
      locationApi.list(),
    ])
      .then(([t, c, m, f, l]) => {
        setTelescopes(t);
        setCameras(c);
        setMounts(m);
        setFilters(f);
        setLocations(l);
      })
      .finally(() => setLoadingEquip(false));
  }, []);

  async function onSubmit(values: FormValues) {
    try {
      const session = await sessionApi.create({
        target_id: values.target_id,
        telescope_id: values.telescope_id,
        camera_id: values.camera_id,
        mount_id: values.mount_id,
        location_id: values.location_id,
        filter_id: values.filter_id || undefined,
        notes: values.notes || undefined,
      });
      router.push(`/observations/${session.id}`);
    } catch (err) {
      form.setError("root", {
        message: err instanceof NyxApiError ? err.message : "Failed to create session.",
      });
    }
  }

  const missingEquip =
    !loadingEquip &&
    (telescopes.length === 0 ||
      cameras.length === 0 ||
      mounts.length === 0 ||
      locations.length === 0);

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
          <div>
            <h1 className="text-base font-semibold">New session</h1>
            <p className="text-sm text-muted-foreground">
              Record a new observation session
            </p>
          </div>
        </div>
      </header>

      <main className="flex-1 p-6">
        <div className="mx-auto max-w-lg">
          {missingEquip && (
            <Alert className="mb-4">
              <AlertDescription>
                You need at least one telescope, camera, mount, and location before
                creating a session.{" "}
                <Link href="/equipment" className="underline">
                  Add equipment
                </Link>{" "}
                or{" "}
                <Link href="/locations" className="underline">
                  add a location
                </Link>
                .
              </AlertDescription>
            </Alert>
          )}

          <Card>
            <CardHeader className="pb-3">
              <CardTitle className="text-sm font-medium">
                Session details
              </CardTitle>
            </CardHeader>
            <CardContent>
              <Form {...form}>
                <form
                  onSubmit={form.handleSubmit(onSubmit)}
                  className="space-y-4"
                >
                  {form.formState.errors.root && (
                    <Alert variant="destructive">
                      <AlertDescription>
                        {form.formState.errors.root.message}
                      </AlertDescription>
                    </Alert>
                  )}

                  {/* Target */}
                  <div className="space-y-1">
                    <p className="text-xs font-medium text-muted-foreground uppercase tracking-wide">
                      Target
                    </p>
                    <FormField
                      control={form.control}
                      name="target_name"
                      render={({ field }) => (
                        <FormItem>
                          <FormLabel>Name</FormLabel>
                          <FormControl>
                            <Input
                              placeholder="e.g. WASP-12 b"
                              {...field}
                              onChange={(e) => {
                                field.onChange(e);
                                if (!e.target.value) {
                                  form.setValue("target_id", "");
                                }
                              }}
                            />
                          </FormControl>
                          <FormMessage />
                          {form.watch("target_id") && (
                            <p className="text-xs text-muted-foreground">
                              ID: {form.watch("target_id")}
                            </p>
                          )}
                        </FormItem>
                      )}
                    />
                    {!form.watch("target_id") && (
                      <p className="text-xs text-muted-foreground">
                        Resolve a target in the{" "}
                        <Link href="/catalog" className="underline">
                          Sky Catalog
                        </Link>{" "}
                        first, then use "New session" from the result.
                      </p>
                    )}
                  </div>

                  {/* Equipment */}
                  <div className="space-y-3">
                    <p className="text-xs font-medium text-muted-foreground uppercase tracking-wide">
                      Equipment
                    </p>
                    <SelectField
                      form={form}
                      name="telescope_id"
                      label="Telescope"
                      items={telescopes}
                      placeholder="Select telescope…"
                    />
                    <SelectField
                      form={form}
                      name="camera_id"
                      label="Camera"
                      items={cameras}
                      placeholder="Select camera…"
                    />
                    <SelectField
                      form={form}
                      name="mount_id"
                      label="Mount"
                      items={mounts}
                      placeholder="Select mount…"
                    />
                    <SelectField
                      form={form}
                      name="filter_id"
                      label="Filter (optional)"
                      items={filters}
                      placeholder="None"
                      optional
                    />
                  </div>

                  {/* Location */}
                  <div className="space-y-3">
                    <p className="text-xs font-medium text-muted-foreground uppercase tracking-wide">
                      Location
                    </p>
                    <SelectField
                      form={form}
                      name="location_id"
                      label="Observing site"
                      items={locations}
                      placeholder="Select location…"
                    />
                  </div>

                  {/* Notes */}
                  <FormField
                    control={form.control}
                    name="notes"
                    render={({ field }) => (
                      <FormItem>
                        <FormLabel>Notes (optional)</FormLabel>
                        <FormControl>
                          <Textarea
                            rows={3}
                            placeholder="Seeing conditions, notes about the session…"
                            {...field}
                          />
                        </FormControl>
                        <FormMessage />
                      </FormItem>
                    )}
                  />

                  <div className="flex justify-end gap-2 pt-1">
                    <Link href="/observations">
                      <Button variant="outline" size="sm" type="button">
                        Cancel
                      </Button>
                    </Link>
                    <Button
                      type="submit"
                      size="sm"
                      disabled={
                        form.formState.isSubmitting ||
                        loadingEquip ||
                        missingEquip
                      }
                    >
                      {form.formState.isSubmitting
                        ? "Creating…"
                        : "Create session"}
                    </Button>
                  </div>
                </form>
              </Form>
            </CardContent>
          </Card>
        </div>
      </main>
    </div>
  );
}

function SelectField({
  form,
  name,
  label,
  items,
  placeholder,
  optional,
}: {
  // eslint-disable-next-line @typescript-eslint/no-explicit-any
  form: any;
  name: string;
  label: string;
  items: { id: string; name: string }[];
  placeholder: string;
  optional?: boolean;
}) {
  return (
    <FormField
      control={form.control}
      name={name}
      render={({ field }: { field: { value: string; onChange: (v: string) => void } }) => (
        <FormItem>
          <FormLabel>{label}</FormLabel>
          <FormControl>
            <select
              value={field.value}
              onChange={(e) => field.onChange(e.target.value)}
              className="flex h-9 w-full rounded-md border border-input bg-transparent px-3 py-1 text-sm shadow-sm transition-colors focus-visible:outline-none focus-visible:ring-1 focus-visible:ring-ring disabled:cursor-not-allowed disabled:opacity-50"
            >
              <option value="">{placeholder}</option>
              {items.map((item) => (
                <option key={item.id} value={item.id}>
                  {item.name}
                </option>
              ))}
            </select>
          </FormControl>
          <FormMessage />
        </FormItem>
      )}
    />
  );
}
