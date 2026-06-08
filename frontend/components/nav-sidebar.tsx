"use client";

import Link from "next/link";
import { usePathname } from "next/navigation";
import {
  BookOpen,
  Eye,
  LayoutDashboard,
  LogOut,
  MapPin,
  Settings,
  Star,
  Telescope,
  Wrench,
} from "lucide-react";
import { cn } from "@/lib/utils";
import { useAuth } from "@/lib/auth-context";
import { ThemeToggle } from "@/components/theme-toggle";
import { Separator } from "@/components/ui/separator";

const navGroups = [
  {
    label: "Observe",
    items: [
      { href: "/dashboard", label: "Dashboard", icon: LayoutDashboard },
      { href: "/catalog", label: "Sky Catalog", icon: Star },
      { href: "/observations", label: "Sessions", icon: Eye },
    ],
  },
  {
    label: "Setup",
    items: [
      { href: "/equipment", label: "Equipment", icon: Wrench },
      { href: "/locations", label: "Locations", icon: MapPin },
    ],
  },
  {
    label: "Journal",
    items: [
      { href: "/journal", label: "Journal", icon: BookOpen },
    ],
  },
];

export function NavSidebar() {
  const pathname = usePathname();
  const { user, logout } = useAuth();

  return (
    <aside className="flex h-screen w-60 shrink-0 flex-col border-r bg-card">
      {/* Logo */}
      <div className="flex h-14 items-center gap-2.5 border-b px-5">
        <Telescope className="h-4 w-4 shrink-0 text-primary" />
        <span className="text-sm font-semibold tracking-tight">Nyx</span>
      </div>

      {/* Nav */}
      <nav className="flex-1 overflow-y-auto px-3 py-3">
        {navGroups.map((group, gi) => (
          <div key={group.label} className={cn(gi > 0 && "mt-4")}>
            <p className="mb-1 px-2 text-[10px] font-medium uppercase tracking-widest text-muted-foreground/60">
              {group.label}
            </p>
            <ul className="space-y-0.5">
              {group.items.map(({ href, label, icon: Icon }) => {
                const active =
                  href === "/dashboard"
                    ? pathname === href
                    : pathname.startsWith(href);
                return (
                  <li key={href}>
                    <Link
                      href={href}
                      className={cn(
                        "flex items-center gap-2.5 rounded-md px-2 py-1.5 text-sm transition-colors",
                        active
                          ? "bg-primary/10 font-medium text-primary"
                          : "text-muted-foreground hover:bg-accent hover:text-foreground",
                      )}
                    >
                      <Icon className="h-3.5 w-3.5 shrink-0" />
                      {label}
                    </Link>
                  </li>
                );
              })}
            </ul>
          </div>
        ))}
      </nav>

      {/* Bottom */}
      <div className="border-t px-3 py-3">
        <div className="mb-2 flex items-center justify-between">
          <Link
            href="/settings"
            className={cn(
              "flex items-center gap-2 rounded-md px-2 py-1.5 text-sm transition-colors",
              pathname === "/settings"
                ? "bg-primary/10 font-medium text-primary"
                : "text-muted-foreground hover:bg-accent hover:text-foreground",
            )}
          >
            <Settings className="h-3.5 w-3.5" />
            Settings
          </Link>
          <div className="flex items-center gap-1">
            <ThemeToggle className="h-7 w-7" />
            <button
              onClick={logout}
              title="Sign out"
              className="flex h-7 w-7 items-center justify-center rounded-md text-muted-foreground transition-colors hover:bg-accent hover:text-foreground"
            >
              <LogOut className="h-3.5 w-3.5" />
            </button>
          </div>
        </div>
        <Separator className="mb-2" />
        <p
          className="truncate px-2 text-xs text-muted-foreground"
          title={user?.email}
        >
          {user?.display_name ?? user?.email}
        </p>
      </div>
    </aside>
  );
}
