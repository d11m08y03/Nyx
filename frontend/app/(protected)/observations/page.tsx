"use client";

import { useEffect, useState } from "react";
import Link from "next/link";
import { Eye, Plus, Search, SortAsc, SortDesc } from "lucide-react";
import { sessionApi, NyxApiError } from "@/lib/api";
import type { ObservationSession } from "@/lib/types";
import { buttonVariants } from "@/components/ui/button";
import { Input } from "@/components/ui/input";
import { Badge } from "@/components/ui/badge";
import { Alert, AlertDescription } from "@/components/ui/alert";
import {
  Table,
  TableBody,
  TableCell,
  TableHead,
  TableHeader,
  TableRow,
} from "@/components/ui/table";
import { Card, CardContent } from "@/components/ui/card";

type SortKey = "created_at" | "target";
type SortDir = "asc" | "desc";

export default function ObservationsPage() {
  const [sessions, setSessions] = useState<ObservationSession[]>([]);
  const [query, setQuery] = useState("");
  const [sortKey, setSortKey] = useState<SortKey>("created_at");
  const [sortDir, setSortDir] = useState<SortDir>("desc");
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState<string | null>(null);

  useEffect(() => {
    sessionApi
      .list()
      .then(setSessions)
      .catch((err) =>
        setError(err instanceof NyxApiError ? err.message : "Failed to load sessions."),
      )
      .finally(() => setLoading(false));
  }, []);

  function toggleSort(key: SortKey) {
    if (sortKey === key) {
      setSortDir((d) => (d === "asc" ? "desc" : "asc"));
    } else {
      setSortKey(key);
      setSortDir("desc");
    }
  }

  const filtered = sessions
    .filter((s) => {
      if (!query) return true;
      const q = query.toLowerCase();
      return (
        s.target_id.toLowerCase().includes(q) ||
        (s.notes ?? "").toLowerCase().includes(q)
      );
    })
    .sort((a, b) => {
      let cmp = 0;
      if (sortKey === "created_at") {
        cmp = new Date(a.created_at).getTime() - new Date(b.created_at).getTime();
      } else if (sortKey === "target") {
        cmp = a.target_id.localeCompare(b.target_id);
      }
      return sortDir === "asc" ? cmp : -cmp;
    });

  function SortIcon({ col }: { col: SortKey }) {
    if (sortKey !== col)
      return <SortAsc className="ml-1 inline h-3 w-3 text-muted-foreground/40" />;
    return sortDir === "asc" ? (
      <SortAsc className="ml-1 inline h-3 w-3" />
    ) : (
      <SortDesc className="ml-1 inline h-3 w-3" />
    );
  }

  return (
    <div className="flex flex-col">
      <header className="border-b px-6 py-4">
        <div className="flex items-center justify-between">
          <div>
            <h1 className="text-base font-semibold">Sessions</h1>
            <p className="text-sm text-muted-foreground">
              All observation sessions
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

      <main className="flex-1 p-6">
        {error && (
          <Alert variant="destructive" className="mb-4">
            <AlertDescription>{error}</AlertDescription>
          </Alert>
        )}

        <div className="mb-4 flex items-center gap-2">
          <div className="relative flex-1 max-w-sm">
            <Search className="absolute left-2.5 top-1/2 h-3.5 w-3.5 -translate-y-1/2 text-muted-foreground" />
            <Input
              className="pl-8 text-sm"
              placeholder="Filter by target, telescope…"
              value={query}
              onChange={(e) => setQuery(e.target.value)}
            />
          </div>
        </div>

        {loading ? (
          <div className="space-y-2">
            {[1, 2, 3, 4].map((i) => (
              <div key={i} className="h-12 animate-pulse rounded-lg bg-muted" />
            ))}
          </div>
        ) : filtered.length === 0 ? (
          <Card className="border-dashed">
            <CardContent className="flex flex-col items-center gap-3 py-12">
              <Eye className="h-8 w-8 text-muted-foreground/30" />
              <p className="text-sm text-muted-foreground">
                {query ? "No sessions match your filter." : "No sessions yet."}
              </p>
              {!query && (
                <Link
                  href="/observations/new"
                  className={buttonVariants({ variant: "outline", size: "sm" })}
                >
                  Create first session
                </Link>
              )}
            </CardContent>
          </Card>
        ) : (
          <Card>
            <Table>
              <TableHeader>
                <TableRow>
                  <TableHead
                    className="cursor-pointer select-none"
                    onClick={() => toggleSort("target")}
                  >
                    Target <SortIcon col="target" />
                  </TableHead>
                  <TableHead>Telescope</TableHead>
                  <TableHead>Camera</TableHead>
                  <TableHead>Location</TableHead>
                  <TableHead>Images</TableHead>
                  <TableHead
                    className="cursor-pointer select-none"
                    onClick={() => toggleSort("created_at")}
                  >
                    Date <SortIcon col="created_at" />
                  </TableHead>
                </TableRow>
              </TableHeader>
              <TableBody>
                {filtered.map((s) => (
                  <TableRow key={s.id} className="cursor-pointer">
                    <TableCell>
                      <Link
                        href={`/observations/${s.id}`}
                        className="block font-mono text-xs hover:text-primary"
                      >
                        {s.target_id.slice(0, 8)}…
                      </Link>
                    </TableCell>
                    <TableCell className="font-mono text-xs text-muted-foreground">
                      {s.telescope_id.slice(0, 8)}…
                    </TableCell>
                    <TableCell className="font-mono text-xs text-muted-foreground">
                      {s.camera_id.slice(0, 8)}…
                    </TableCell>
                    <TableCell className="font-mono text-xs text-muted-foreground">
                      {s.location_id.slice(0, 8)}…
                    </TableCell>
                    <TableCell>
                      {s.images.length > 0 ? (
                        <Badge variant="secondary" className="text-xs">
                          {s.images.length}
                        </Badge>
                      ) : (
                        <span className="text-muted-foreground text-sm">—</span>
                      )}
                    </TableCell>
                    <TableCell className="text-muted-foreground text-sm">
                      {new Date(s.created_at).toLocaleDateString()}
                    </TableCell>
                  </TableRow>
                ))}
              </TableBody>
            </Table>
          </Card>
        )}
      </main>
    </div>
  );
}
