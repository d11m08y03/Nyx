"use client";

import { useState } from "react";
import Link from "next/link";
import { useRouter } from "next/navigation";
import { useForm } from "react-hook-form";
import { zodResolver } from "@hookform/resolvers/zod";
import { z } from "zod";
import { Search, Star } from "lucide-react";
import { targetApi, NyxApiError } from "@/lib/api";
import type { Target } from "@/lib/types";
import {
  Card,
  CardContent,
  CardDescription,
  CardHeader,
  CardTitle,
} from "@/components/ui/card";
import {
  Form,
  FormControl,
  FormField,
  FormItem,
  FormMessage,
} from "@/components/ui/form";
import { Input } from "@/components/ui/input";
import { Button } from "@/components/ui/button";
import { Alert, AlertDescription } from "@/components/ui/alert";
import { Badge } from "@/components/ui/badge";

const schema = z.object({
  target_name: z.string().min(1, "Enter a target name"),
});
type FormValues = z.infer<typeof schema>;

export default function CatalogPage() {
  const router = useRouter();
  const [result, setResult] = useState<Target | null>(null);
  const [error, setError] = useState<string | null>(null);

  const form = useForm<FormValues>({
    resolver: zodResolver(schema),
    defaultValues: { target_name: "" },
  });

  async function onSubmit(values: FormValues) {
    setError(null);
    setResult(null);
    try {
      const target = await targetApi.resolve(values.target_name);
      setResult(target);
    } catch (err) {
      setError(
        err instanceof NyxApiError ? err.message : "Failed to resolve target.",
      );
    }
  }

  return (
    <div className="flex flex-col">
      <header className="border-b px-6 py-4">
        <h1 className="text-base font-semibold">Sky Catalog</h1>
        <p className="text-sm text-muted-foreground">
          Resolve targets from NASA MAST and Exoplanet Archive
        </p>
      </header>

      <main className="flex-1 p-6">
        <div className="mx-auto max-w-2xl space-y-6">
          <Card>
            <CardHeader className="pb-3">
              <CardTitle className="text-sm font-medium">
                Target search
              </CardTitle>
              <CardDescription className="text-xs">
                Enter a star name, exoplanet, or catalogue ID (e.g. "WASP-12",
                "TIC 123456789", "Kepler-452")
              </CardDescription>
            </CardHeader>
            <CardContent>
              <Form {...form}>
                <form
                  onSubmit={form.handleSubmit(onSubmit)}
                  className="flex gap-2"
                >
                  <FormField
                    control={form.control}
                    name="target_name"
                    render={({ field }) => (
                      <FormItem className="flex-1">
                        <FormControl>
                          <Input
                            placeholder="e.g. WASP-12 b"
                            autoComplete="off"
                            {...field}
                          />
                        </FormControl>
                        <FormMessage />
                      </FormItem>
                    )}
                  />
                  <Button
                    type="submit"
                    disabled={form.formState.isSubmitting}
                    className="shrink-0"
                  >
                    <Search className="mr-1.5 h-3.5 w-3.5" />
                    {form.formState.isSubmitting ? "Resolving…" : "Resolve"}
                  </Button>
                </form>
              </Form>
            </CardContent>
          </Card>

          {error && (
            <Alert variant="destructive">
              <AlertDescription>{error}</AlertDescription>
            </Alert>
          )}

          {result && (
            <Card>
              <CardHeader className="pb-2">
                <div className="flex items-start justify-between gap-3">
                  <div className="flex items-center gap-2">
                    <Star className="h-4 w-4 text-primary" />
                    <CardTitle className="text-base">
                      {result.canonical_name}
                    </CardTitle>
                  </div>
                  {result.target_type && (
                    <Badge variant="secondary" className="shrink-0 text-xs">
                      {result.target_type}
                    </Badge>
                  )}
                </div>
              </CardHeader>
              <CardContent className="space-y-3">
                <div className="grid grid-cols-2 gap-x-6 gap-y-1.5 text-sm">
                  {result.right_ascension != null && (
                    <>
                      <span className="text-muted-foreground">RA</span>
                      <span className="font-mono">
                        {result.right_ascension.toFixed(6)}°
                      </span>
                    </>
                  )}
                  {result.declination != null && (
                    <>
                      <span className="text-muted-foreground">Dec</span>
                      <span className="font-mono">
                        {result.declination.toFixed(6)}°
                      </span>
                    </>
                  )}
                  {result.tess_observations != null && (
                    <>
                      <span className="text-muted-foreground">
                        TESS observations
                      </span>
                      <span>{result.tess_observations.length}</span>
                    </>
                  )}
                </div>
                <div className="flex gap-2 pt-1">
                  <Button
                    size="sm"
                    onClick={() => router.push(`/targets/${result.id}`)}
                  >
                    View light curves
                  </Button>
                  <Button
                    variant="outline"
                    size="sm"
                    onClick={() =>
                      router.push(
                        `/observations/new?target_id=${result.id}&target_name=${encodeURIComponent(result.canonical_name)}`,
                      )
                    }
                  >
                    New session
                  </Button>
                </div>
              </CardContent>
            </Card>
          )}
        </div>
      </main>
    </div>
  );
}
