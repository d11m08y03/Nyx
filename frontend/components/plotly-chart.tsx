"use client";

import dynamic from "next/dynamic";
import type { Data, Layout, Config } from "plotly.js";

const Plot = dynamic(() => import("react-plotly.js"), {
  ssr: false,
  loading: () => (
    <div className="flex h-64 items-center justify-center text-sm text-muted-foreground">
      Loading chart…
    </div>
  ),
});

interface PlotlyChartProps {
  data: Data[];
  layout?: Partial<Layout>;
  className?: string;
}

const darkConfig: Partial<Layout> = {
  paper_bgcolor: "transparent",
  plot_bgcolor: "transparent",
  font: { color: "#a1a1aa", size: 11, family: "inherit" },
  xaxis: {
    gridcolor: "rgba(255,255,255,0.06)",
    linecolor: "rgba(255,255,255,0.1)",
    zerolinecolor: "rgba(255,255,255,0.1)",
    tickfont: { color: "#71717a" },
  },
  yaxis: {
    gridcolor: "rgba(255,255,255,0.06)",
    linecolor: "rgba(255,255,255,0.1)",
    zerolinecolor: "rgba(255,255,255,0.1)",
    tickfont: { color: "#71717a" },
  },
  legend: {
    bgcolor: "rgba(0,0,0,0)",
    font: { color: "#a1a1aa" },
  },
  margin: { l: 50, r: 20, t: 30, b: 50 },
};

const plotConfig: Partial<Config> = {
  displaylogo: false,
  modeBarButtonsToRemove: [
    "sendDataToCloud",
    "lasso2d",
    "select2d",
    "toImage",
  ],
  responsive: true,
};

export function PlotlyChart({ data, layout, className }: PlotlyChartProps) {
  return (
    <div className={className}>
      <Plot
        data={data}
        layout={{ ...darkConfig, ...layout, autosize: true }}
        config={plotConfig}
        style={{ width: "100%", height: "100%" }}
        useResizeHandler
      />
    </div>
  );
}
