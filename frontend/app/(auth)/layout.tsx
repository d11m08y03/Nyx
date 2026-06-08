import { Telescope } from "lucide-react";
import { ThemeToggle } from "@/components/theme-toggle";

export default function AuthLayout({
  children,
}: {
  children: React.ReactNode;
}) {
  return (
    <div className="relative flex min-h-screen flex-col items-center justify-center overflow-hidden p-4">
      {/* Space background — dark mode only */}
      <div className="pointer-events-none absolute inset-0 -z-10">
        <div className="absolute inset-0 bg-background" />
        <div className="dark:block hidden absolute inset-0">
          {/* Deep space gradient */}
          <div className="absolute inset-0 bg-[radial-gradient(ellipse_80%_50%_at_50%_-10%,oklch(0.25_0.06_270/0.4),transparent)]" />
          <div className="absolute inset-0 bg-[radial-gradient(ellipse_40%_60%_at_80%_80%,oklch(0.2_0.08_220/0.3),transparent)]" />
          {/* Star field layers */}
          <div
            className="star-layer-1 absolute inset-0 opacity-60"
            style={{
              backgroundImage:
                "radial-gradient(circle, oklch(0.95 0 0 / 0.8) 1px, transparent 1px)",
              backgroundSize: "130px 130px",
            }}
          />
          <div
            className="star-layer-2 absolute inset-0 opacity-40"
            style={{
              backgroundImage:
                "radial-gradient(circle, oklch(0.95 0 0 / 0.7) 1px, transparent 1px)",
              backgroundSize: "80px 80px",
              backgroundPosition: "40px 65px",
            }}
          />
          <div
            className="star-layer-3 absolute inset-0 opacity-25"
            style={{
              backgroundImage:
                "radial-gradient(circle, oklch(0.95 0 0 / 0.6) 1px, transparent 1px)",
              backgroundSize: "200px 200px",
              backgroundPosition: "10px 100px",
            }}
          />
        </div>
        {/* Light mode subtle grid */}
        <div
          className="dark:hidden absolute inset-0 opacity-30"
          style={{
            backgroundImage:
              "radial-gradient(circle, oklch(0.4 0 0 / 0.3) 1px, transparent 1px)",
            backgroundSize: "24px 24px",
          }}
        />
      </div>

      {/* Theme toggle */}
      <div className="absolute right-4 top-4">
        <ThemeToggle />
      </div>

      {/* Logo */}
      <div className="mb-8 flex flex-col items-center gap-1 text-center">
        <div className="mb-1 flex items-center gap-2">
          <Telescope className="h-5 w-5 text-primary" />
          <span className="text-2xl font-bold tracking-tight">Nyx</span>
        </div>
        <p className="text-xs text-muted-foreground">
          Astronomical observation platform
        </p>
      </div>

      <div className="w-full max-w-sm">{children}</div>
    </div>
  );
}
