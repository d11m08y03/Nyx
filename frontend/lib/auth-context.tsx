"use client";

import React, {
  createContext,
  useCallback,
  useContext,
  useEffect,
  useRef,
  useState,
} from "react";
import { useRouter } from "next/navigation";
import {
  authApi,
  NyxApiError,
  setAccessToken,
  setUnauthenticatedHandler,
} from "@/lib/api";
import type { UserProfile } from "@/lib/types";

function decodeJwtPayload(token: string): Record<string, unknown> {
  try {
    const payload = token.split(".")[1];
    return JSON.parse(atob(payload.replace(/-/g, "+").replace(/_/g, "/")));
  } catch {
    return {};
  }
}

interface AuthContextValue {
  user: UserProfile | null;
  accessToken: string | null;
  isLoading: boolean;
  login: (email: string, password: string) => Promise<void>;
  register: (email: string, password: string) => Promise<UserProfile>;
  logout: () => Promise<void>;
  setUser: React.Dispatch<React.SetStateAction<UserProfile | null>>;
}

const AuthContext = createContext<AuthContextValue | null>(null);

export function AuthProvider({ children }: { children: React.ReactNode }) {
  const [user, setUser] = useState<UserProfile | null>(null);
  const [accessToken, setToken] = useState<string | null>(null);
  const [isLoading, setIsLoading] = useState(true);
  const router = useRouter();
  const refreshTimerRef = useRef<ReturnType<typeof setTimeout> | null>(null);

  const storeToken = useCallback((token: string) => {
    setAccessToken(token);
    setToken(token);
    const payload = decodeJwtPayload(token);
    setUser({
      id: payload.sub as string,
      email: payload.email as string,
      email_verified: (payload.email_verified as boolean) ?? false,
    });
    const exp = payload.exp as number | undefined;
    if (exp) {
      const msUntilExpiry = exp * 1000 - Date.now() - 30_000;
      if (refreshTimerRef.current) clearTimeout(refreshTimerRef.current);
      if (msUntilExpiry > 0) {
        refreshTimerRef.current = setTimeout(silentRefresh, msUntilExpiry);
      }
    }
  }, []);

  const clearAuth = useCallback(() => {
    setAccessToken(null);
    setToken(null);
    setUser(null);
    if (refreshTimerRef.current) clearTimeout(refreshTimerRef.current);
  }, []);

  const silentRefresh = useCallback(async () => {
    try {
      const data = await authApi.refresh();
      storeToken(data.access_token);
    } catch {
      clearAuth();
    }
  }, [storeToken, clearAuth]);

  useEffect(() => {
    setUnauthenticatedHandler(() => {
      clearAuth();
      router.replace("/login");
    });
  }, [clearAuth, router]);

  useEffect(() => {
    silentRefresh().finally(() => setIsLoading(false));
    return () => {
      if (refreshTimerRef.current) clearTimeout(refreshTimerRef.current);
    };
  }, []);

  const login = useCallback(
    async (email: string, password: string) => {
      const data = await authApi.login(email, password);
      storeToken(data.access_token);
    },
    [storeToken],
  );

  const register = useCallback(
    async (email: string, password: string): Promise<UserProfile> => {
      return authApi.register(email, password);
    },
    [],
  );

  const logout = useCallback(async () => {
    try {
      await authApi.logout();
    } finally {
      clearAuth();
    }
  }, [clearAuth]);

  return (
    <AuthContext.Provider
      value={{ user, accessToken, isLoading, login, register, logout, setUser }}
    >
      {children}
    </AuthContext.Provider>
  );
}

export function useAuth(): AuthContextValue {
  const ctx = useContext(AuthContext);
  if (!ctx) throw new Error("useAuth must be used inside AuthProvider");
  return ctx;
}
