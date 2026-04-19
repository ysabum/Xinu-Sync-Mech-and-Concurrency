library(tidyverse)

# Load both datasets and tag them
base <- read_csv("baseline.csv") %>% mutate(locking = "coarse")
fine <- read_csv("finegrained.csv") %>% mutate(locking = "fine")

# Combine
df <- bind_rows(base, fine)

# Convert op to factor
df$op <- factor(df$op,
                levels = c(1,2,3,4),
                labels = c("OPEN","READ","WRITE","CLOSE"))

# Filter to real per-op samples
df3 <- df %>% filter(op %in% c("READ","WRITE"))

# Create combined category for legend
df3 <- df3 %>%
  mutate(category = paste0(locking, "_", op))  # e.g., "coarse_READ", "fine_WRITE"

# Optional zoom to remove extreme outliers
df_zoom <- df3 %>% filter(latency_ticks < 200000)

# Unified overlay plot
ggplot(df_zoom, aes(x = cs_ticks, y = latency_ticks, color = category)) +
  geom_point(alpha = 0.25) +
  stat_smooth(method = "lm", formula = y ~ x, se = FALSE, size = 1.1) +
  scale_color_manual(
    values = c(
      "coarse_READ"  = "#d73027",  # red
      "coarse_WRITE" = "#4575b4",  # blue
      "fine_READ"    = "#fc8d59",  # light red/orange
      "fine_WRITE"   = "#91bfdb"   # light blue
    )
  ) +
  labs(
    title = "Latency vs Critical Section Time (Coarse vs Fine-Grained Locking)",
    x = "Critical Section Time (ticks)",
    y = "Latency (ticks)",
    color = "Locking + Operation"
  ) +
  theme_minimal()

ggsave("combined_latency_vs_cs_overlay_4cat.png", width = 10, height = 7, dpi = 300)