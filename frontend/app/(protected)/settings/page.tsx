"use client";

import { useEffect } from "react";
import { useForm } from "react-hook-form";
import { zodResolver } from "@hookform/resolvers/zod";
import { z } from "zod";
import { useAuth } from "@/lib/auth-context";
import { profileApi, NyxApiError } from "@/lib/api";
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
import { Separator } from "@/components/ui/separator";

const profileSchema = z.object({
  display_name: z
    .string()
    .min(2, "Minimum 2 characters")
    .max(64, "Maximum 64 characters"),
});

type FormValues = z.infer<typeof profileSchema>;

export default function SettingsPage() {
  const { user, setUser } = useAuth();

  const form = useForm<FormValues>({
    resolver: zodResolver(profileSchema),
    defaultValues: { display_name: user?.display_name ?? "" },
  });

  useEffect(() => {
    form.reset({ display_name: user?.display_name ?? "" });
  }, [user, form]);

  async function onSubmit(values: FormValues) {
    try {
      const updated = await profileApi.updateDisplayName(values.display_name);
      setUser((prev) => prev ? { ...prev, display_name: updated.display_name } : prev);
      form.reset({ display_name: updated.display_name });
    } catch (err) {
      form.setError("root", {
        message: err instanceof NyxApiError ? err.message : "Save failed.",
      });
    }
  }

  return (
    <div className="flex flex-col">
      <header className="border-b px-6 py-4">
        <h1 className="text-base font-semibold">Settings</h1>
        <p className="text-sm text-muted-foreground">
          Manage your profile and preferences
        </p>
      </header>

      <main className="flex-1 p-6">
        <div className="mx-auto max-w-lg space-y-6">
          <Card>
            <CardHeader className="pb-3">
              <CardTitle className="text-sm font-medium">Profile</CardTitle>
            </CardHeader>
            <CardContent>
              <div className="mb-4 space-y-1 text-sm">
                <p className="text-muted-foreground">Email</p>
                <p className="font-mono">{user?.email}</p>
              </div>
              <Separator className="mb-4" />
              <Form {...form}>
                <form
                  onSubmit={form.handleSubmit(onSubmit)}
                  className="space-y-3"
                >
                  {form.formState.errors.root && (
                    <Alert variant="destructive">
                      <AlertDescription>
                        {form.formState.errors.root.message}
                      </AlertDescription>
                    </Alert>
                  )}
                  {form.formState.isSubmitSuccessful && (
                    <Alert>
                      <AlertDescription>
                        Profile updated.
                      </AlertDescription>
                    </Alert>
                  )}
                  <FormField
                    control={form.control}
                    name="display_name"
                    render={({ field }) => (
                      <FormItem>
                        <FormLabel>Display name</FormLabel>
                        <FormControl>
                          <Input placeholder="Your name" {...field} />
                        </FormControl>
                        <FormMessage />
                      </FormItem>
                    )}
                  />
                  <Button
                    type="submit"
                    size="sm"
                    disabled={form.formState.isSubmitting || !form.formState.isDirty}
                  >
                    {form.formState.isSubmitting ? "Saving…" : "Save changes"}
                  </Button>
                </form>
              </Form>
            </CardContent>
          </Card>
        </div>
      </main>
    </div>
  );
}
