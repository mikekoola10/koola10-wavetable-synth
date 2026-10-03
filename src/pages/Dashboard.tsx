import { useAuth } from "@/hooks/use-auth";
import { LogOut } from "lucide-react";
import { NavLink, Outlet, useNavigate } from "react-router";

import { Button } from "@/components/ui/button";
import { cn } from "@/lib/utils";

const NAV = [
  { to: "/dashboard", label: "Patch library", end: true },
  { to: "/dashboard/signal-chain", label: "Signal chain", end: false },
  { to: "/dashboard/build-guide", label: "Build guide", end: false },
];

function navClass({ isActive }: { isActive: boolean }) {
  return cn(
    "block border-l-2 py-2 pl-4 pr-3 text-[13px] transition-colors",
    isActive
      ? "border-foreground font-medium text-foreground"
      : "border-transparent text-muted-foreground hover:border-border hover:text-foreground",
  );
}

export default function Dashboard() {
  const { user, signOut } = useAuth();
  const navigate = useNavigate();

  const handleSignOut = async () => {
    await signOut();
    navigate("/");
  };

  return (
    <div className="dark min-h-screen bg-background text-foreground">
      <header className="border-b border-border">
        <div className="mx-auto flex h-14 w-full max-w-6xl items-center justify-between px-6">
          <NavLink to="/" className="flex items-center gap-2">
            <span className="size-4 border border-foreground" />
            <span className="text-sm font-bold tracking-tight">Koola10 Synth</span>
          </NavLink>
          <span className="label-mono hidden sm:block">Patch studio</span>
        </div>
      </header>

      <div className="mx-auto flex w-full max-w-6xl flex-col px-6 lg:flex-row lg:gap-12">
        {/* Sidebar on wide screens, horizontal nav strip on narrow ones. */}
        <aside className="shrink-0 border-b border-border py-6 lg:w-56 lg:border-b-0 lg:py-12">
          <p className="label-mono hidden lg:block">Studio</p>

          <nav className="mt-0 flex gap-2 overflow-x-auto pb-1 lg:mt-6 lg:flex-col lg:gap-0 lg:overflow-visible lg:pb-0">
            {NAV.map((item) => (
              <NavLink key={item.to} to={item.to} end={item.end} className={navClass}>
                {item.label}
              </NavLink>
            ))}
          </nav>

          <div className="mt-8 hidden border-t border-border pt-6 lg:block">
            <p className="label-mono">Signed in</p>
            <p className="mt-2 truncate text-[13px] text-muted-foreground">
              {user?.name || user?.email || "Guest"}
            </p>
            <Button
              type="button"
              variant="ghost"
              className="mt-3 -ml-3 rounded-none gap-2 text-muted-foreground hover:text-foreground"
              onClick={handleSignOut}
            >
              <LogOut className="size-3.5" />
              Sign out
            </Button>
          </div>
        </aside>

        <main className="min-w-0 flex-1 py-10 lg:py-12">
          <Outlet />
        </main>
      </div>

      <div className="mx-auto flex w-full max-w-6xl items-center justify-between border-t border-border px-6 py-6 lg:hidden">
        <span className="label-mono truncate">
          {user?.name || user?.email || "Guest"}
        </span>
        <Button
          type="button"
          variant="ghost"
          size="sm"
          className="rounded-none gap-2 text-muted-foreground"
          onClick={handleSignOut}
        >
          <LogOut className="size-3.5" />
          Sign out
        </Button>
      </div>
    </div>
  );
}
