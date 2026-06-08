"use client";

import { useEffect, useState } from "react";
import Link from "next/link";
import { Eye, Plus, Star, Telescope, TrendingUp } from "lucide-react";
import { useAuth } from "@/lib/auth-context";
import { sessionApi, telescopeApi, locationApi } from "@/lib/api";
import type { ObservationSession, Telescope as TelescopeType, ObservingLocation } from "@/lib/types";
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from "@/components/ui/card";
import { Badge } from "@/components/ui/badge";
import { Button } from "@/components/ui/button";
import { buttonVariants } from "@/components/ui/button";

export default function DashboardPage() {
  const { user } = useAuth();
  const [sessions, setSessions] = useState<ObservationSession[]>([]);
  const [telescopes, setTelescopes] = useState<TelescopeType[]>([]);
  const [locations, setLocations] = useState<ObservingLocation[]>([]);
  const [loading, setLoading] = useState(true);

  useEffect(() => {
    Promise.all([
      sessionApi.list().catch(() => [] as ObservationSession[]),
      telescopeApi.list().catch(() => [] as TelescopeType[]),
      locationApi.list().catch(() => [] as ObservingLocation[]),
    ]).then(([s, t, l]) => {
      setSessions(s);
      setTelescopes(t);
      setLocations(l);
      setLoading(false);
    });
  }, []);

  const targets = new Set(sessions.map((s) => s.target_id)).size;

  const stats = [
    { label: "Sessions", value: loading ? "—" : sessions.length, icon: Eye, href: "/observations" },
    { label: "Targets", value: loading ? "—" : targets, icon: Star, href: "/catalog" },
    { label: "Telescopes", value: loading ? "—" : telescopes.length, icon: Telescope, href: "/equipment" },
    { label: "Locations", value: loading ? "—" : locations.length, icon: TrendingUp, href: "/locations" },
  ];

  return (
    <div className="flex flex-col">
      <header className="border-b px-6 py-4">
        <div className="flex items-center justify-between">
          <div>
            <h1 className="text-base font-semibold">Dashboard</h1>
            <p className="text-sm text-muted-foreground">
              {user?.display_name ?? user?.email}
            </p>
          </div>
          <Link
            href="/observations/new"
            className={buttonVariants({ size: "sm" })}
          >
            <Plus className="mr-1.5 h-3.5 w-3.5" />
            New session
          </Link>
        </div>
      </header>

      <main className="flex-1 space-y-6 p-6">
        {/* Stats */}
        <div className="grid grid-cols-2 gap-3 lg:grid-cols-4">
          {stats.map(({ label, value, icon: Icon, href }) => (
            <Link key={label} href={href}>
              <Card className="transition-colors hover:bg-accent/50">
                <CardHeader className="pb-1 pt-4">
                  <div className="flex items-center justify-between">
                    <CardDescription className="text-xs">{label}</CardDescription>
                    <Icon className="h-3.5 w-3.5 text-muted-foreground/50" />
                  </div>
                  <CardTitle className="text-2xl font-bold">{value}</CardTitle>
                </CardHeader>
              </Card>
            </Link>
          ))}
        </div>

        {/* Recent sessions */}
        <section>
          <div className="mb-3 flex items-center justify-between">
            <h2 className="text-sm font-medium">Recent sessions</h2>
            <Link
              href="/observations"
              className="text-xs text-muted-foreground hover:text-foreground"
            >
              View all
            </Link>
          </div>
          {loading ? (
            <div className="space-y-2">
              {[1, 2, 3].map((i) => (
                <div key={i} className="h-14 animate-pulse rounded-lg bg-muted" />
              ))}
            </div>
          ) : sessions.length === 0 ? (
            <Card className="border-dashed">
              <CardContent className="flex flex-col items-center gap-3 py-10">
                <Eye className="h-8 w-8 text-muted-foreground/40" />
                <p className="text-sm text-muted-foreground">No sessions yet</p>
                <Link
                  href="/observations/new"
                  className={buttonVariants({ variant: "outline", size: "sm" })}
                >
                  Start your first session
                </Link>
              </CardContent>
            </Card>
          ) : (
            <div className="space-y-2">
              {sessions.slice(0, 5).map((s) => (
                <Link key={s.id} href={`/observations/${s.id}`}>
                  <Card className="transition-colors hover:bg-accent/50">
                    <CardContent className="flex items-center justify-between py-3 px-4">
                      <div className="min-w-0">
                        <p className="truncate text-sm font-medium font-mono text-xs">
                          {s.target_id.slice(0, 8)}…
                        </p>
                        <p className="truncate text-xs text-muted-foreground">
                          {new Date(s.created_at).toLocaleDateString()}
                        </p>
                      </div>
                      <div className="ml-4 shrink-0 text-right">
                        <p className="text-xs text-muted-foreground">
                          {new Date(s.created_at).toLocaleDateString()}
                        </p>
                        {s.images.length > 0 && (
                          <Badge variant="secondary" className="text-[10px]">
                            {s.images.length} images
                          </Badge>
                        )}
                      </div>
                    </CardContent>
                  </Card>
                </Link>
              ))}
            </div>
          )}
        </section>
      </main>
    </div>
  );
}
