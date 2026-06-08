"use client";

import Link from "next/link";
import { BookOpen } from "lucide-react";
import { buttonVariants } from "@/components/ui/button";
import { Card, CardContent } from "@/components/ui/card";

export default function JournalPage() {
  return (
    <div className="flex flex-col">
      <header className="border-b px-6 py-4">
        <h1 className="text-base font-semibold">Journal</h1>
        <p className="text-sm text-muted-foreground">
          Timeline of your observations
        </p>
      </header>

      <main className="flex-1 p-6">
        <Card className="border-dashed">
          <CardContent className="flex flex-col items-center gap-3 py-16">
            <BookOpen className="h-8 w-8 text-muted-foreground/30" />
            <p className="text-sm text-muted-foreground">
              Journal view coming soon.
            </p>
            <Link
              href="/observations"
              className={buttonVariants({ variant: "outline", size: "sm" })}
            >
              View sessions
            </Link>
          </CardContent>
        </Card>
      </main>
    </div>
  );
}
