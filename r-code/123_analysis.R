library(ggplot2)
library(dplyr)
library(readr)

library(tidyverse)
df <- read_csv("baseline.csv")
df$op <- factor(df$op,
                levels = c(1,2,3,4),
                labels = c("OPEN","READ","WRITE","CLOSE"))

# Latency vs Workers (RQ1)
latency_by_worker <- df %>%
  group_by(pid) %>%
  summarise(mean_latency = mean(latency_ticks))

ggplot(latency_by_worker, aes(x = pid, y = mean_latency)) +
  geom_line() +
  geom_point(size = 2) +
  labs(title = "Mean Latency per Worker",
       x = "Worker PID",
       y = "Mean Latency (ticks)") +
  theme_minimal()


# Throughput vs Workers (RQ1)
throughput_by_worker <- df %>%
  group_by(pid) %>%
  summarise(ops = n())

ggplot(throughput_by_worker, aes(x = pid, y = ops)) +
  geom_col(fill = "steelblue") +
  labs(title = "Throughput per Worker",
       x = "Worker PID",
       y = "Total Operations") +
  theme_minimal()

# Latency Histogram by Operation (RQ2)
ggplot(df, aes(x = latency_ticks, fill = op)) +
  geom_histogram(bins = 60, alpha = 0.6, position = "identity") +
  scale_x_continuous(labels = scales::comma) +
  labs(title = "Latency Distribution by Operation Type",
       x = "Latency (ticks)",
       y = "Count",
       fill = "Operation") +
  theme_minimal()

# Boxplot of Read vs Write Latency (RQ2)
df_rw <- df %>% filter(op %in% c("READ","WRITE"))

ggplot(df_rw, aes(x = op, y = latency_ticks, fill = op)) +
  geom_boxplot(alpha = 0.7) +
  scale_y_continuous(labels = scales::comma) +
  labs(title = "Latency Comparison: READ vs WRITE",
       x = "Operation",
       y = "Latency (ticks)") +
  theme_minimal()

# Scatterplot: Latency vs Critical Section Time (RQ3)
ggplot(df, aes(x = cs_ticks, y = latency_ticks, color = op)) +
  geom_point(alpha = 0.4) +
  geom_smooth(method = "lm", se = FALSE) +
  scale_x_continuous(labels = scales::comma) +
  scale_y_continuous(labels = scales::comma) +
  labs(title = "Latency vs Critical Section Time",
       x = "Critical Section Time (ticks)",
       y = "Latency (ticks)",
       color = "Operation") +
  theme_minimal()

# Correlation Between Latency and Critical Section Time (RQ3)
cor(df$latency_ticks, df$cs_ticks)

df_write <- df %>% filter(op == "WRITE")
cor(df_write$latency_ticks, df_write$cs_ticks)

# Histogram of Critical Section Time (RQ3)
ggplot(df, aes(x = cs_ticks)) +
  geom_histogram(bins = 60, fill = "darkred", alpha = 0.7) +
  scale_x_continuous(labels = scales::comma) +
  labs(title = "Distribution of Critical Section Time",
       x = "Critical Section Time (ticks)",
       y = "Count") +
  theme_minimal()

# Bimodal Write Latency Visualization (RQ2)
df_write <- df %>% filter(op == "WRITE")

ggplot(df_write, aes(x = latency_ticks)) +
  geom_density(fill = "purple", alpha = 0.5) +
  scale_x_continuous(labels = scales::comma) +
  labs(title = "Density Plot of WRITE Latency (Bimodal Distribution)",
       x = "Latency (ticks)",
       y = "Density") +
  theme_minimal()