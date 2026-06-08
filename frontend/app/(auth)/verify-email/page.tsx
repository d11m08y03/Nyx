"use client";

import { Suspense, useEffect, useState } from "react";
import { useSearchParams } from "next/navigation";
import Link from "next/link";
import { authApi, NyxApiError } from "@/lib/api";
import {
  Card,
  CardContent,
  CardDescription,
  CardHeader,
  CardTitle,
} from "@/components/ui/card";
import { Alert, AlertDescription } from "@/components/ui/alert";
import { buttonVariants } from "@/components/ui/button";

function VerifyEmailContent() {
  const params = useSearchParams();
  const token = params.get("token");
  const registered = params.get("registered");
  const email = params.get("email");

  const [status, setStatus] = useState<"idle" | "verifying" | "success" | "error">(
    token ? "verifying" : "idle",
  );
  const [message, setMessage] = useState<string | null>(null);

  useEffect(() => {
    if (!token) return;
    setStatus("verifying");
    authApi
      .verifyEmail(token)
      .then((data) => {
        setStatus("success");
        setMessage(data.message);
      })
      .catch((err) => {
        setStatus("error");
        setMessage(
          err instanceof NyxApiError ? err.message : "Verification failed.",
        );
      });
  }, [token]);

  if (status === "verifying") {
    return (
      <Card>
        <CardHeader>
          <CardTitle>Verifying…</CardTitle>
        </CardHeader>
        <CardContent>
          <p className="text-sm text-muted-foreground">
            Checking your verification link.
          </p>
        </CardContent>
      </Card>
    );
  }

  if (status === "success") {
    return (
      <Card>
        <CardHeader>
          <CardTitle>Email verified</CardTitle>
          <CardDescription>{message}</CardDescription>
        </CardHeader>
        <CardContent>
          <Link href="/login" className={buttonVariants({ className: "w-full" })}>
            Sign in
          </Link>
        </CardContent>
      </Card>
    );
  }

  if (status === "error") {
    return (
      <Card>
        <CardHeader>
          <CardTitle>Verification failed</CardTitle>
        </CardHeader>
        <CardContent className="space-y-4">
          <Alert variant="destructive">
            <AlertDescription>{message}</AlertDescription>
          </Alert>
          <Link
            href="/resend-verification"
            className={buttonVariants({ variant: "outline", className: "w-full" })}
          >
            Resend verification email
          </Link>
        </CardContent>
      </Card>
    );
  }

  return (
    <Card>
      <CardHeader>
        <CardTitle>Check your inbox</CardTitle>
        <CardDescription>
          {registered
            ? `We sent a verification link to${email ? ` ${email}` : " your email"}.`
            : "Follow the link in your email to verify your account."}
        </CardDescription>
      </CardHeader>
      <CardContent className="space-y-4">
        <p className="text-sm text-muted-foreground">
          Didn&apos;t receive it?{" "}
          <Link
            href={`/resend-verification${email ? `?email=${encodeURIComponent(email)}` : ""}`}
            className="text-foreground underline underline-offset-2"
          >
            Resend verification email
          </Link>
        </p>
        <p className="text-sm text-muted-foreground">
          <Link href="/login" className="text-foreground underline underline-offset-2">
            Back to sign in
          </Link>
        </p>
      </CardContent>
    </Card>
  );
}

export default function VerifyEmailPage() {
  return (
    <Suspense>
      <VerifyEmailContent />
    </Suspense>
  );
}
