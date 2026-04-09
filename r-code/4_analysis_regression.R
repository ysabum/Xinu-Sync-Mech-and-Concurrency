library(tidyverse)

df <- read_csv("finegrained.csv")

# Convert op to factor
df$op <- factor(df$op,
                levels = c(1,2,3,4),
                labels = c("OPEN","READ","WRITE","CLOSE"))

# Filter to real per-op samples
df3 <- df %>% filter(op %in% c("READ","WRITE"))

# Unified regression model
model <- lm(latency_ticks ~ cs_ticks + op + pid, data = df3)
summary(model)

# Optional interaction model
model_interact <- lm(latency_ticks ~ cs_ticks * op + pid, data = df3)
summary(model_interact)

# Export summaries
model_summary <- as.data.frame(summary(model)$coefficients)
model_summary$model <- "Main Model"

model_interact_summary <- as.data.frame(summary(model_interact)$coefficients)
model_interact_summary$model <- "Interaction Model"

all_models <- rbind(model_summary, model_interact_summary)
write.csv(all_models, "finegrained_regression_summaries.csv")

# Plot
df_zoom <- df3 %>% filter(latency_ticks < 200000)

ggplot(df_zoom, aes(x = cs_ticks, y = latency_ticks, color = op)) +
  geom_point(alpha = 0.3) +
  stat_smooth(method = "lm", formula = y ~ x, se = FALSE) +
  labs(
    title = "Latency vs Critical Section Time (Fine-Grained Locking)",
    x = "Critical Section Time (ticks)",
    y = "Latency (ticks)"
  ) +
  theme_minimal()

ggsave("finegrained_latency_vs_cs_unified.png", width = 8, height = 6, dpi = 300)

ggplot(df3, aes(x = latency_ticks, fill = op)) +
  geom_density(alpha = 0.4) +
  scale_x_log10() +
  labs(
    title = "Latency Distribution (Fine-Grained Locking)",
    x = "Latency (ticks, log scale)",
    y = "Density"
  ) +
  theme_minimal()

ggsave("finegrained_latency_distribution.png", width = 8, height = 6, dpi = 300)
