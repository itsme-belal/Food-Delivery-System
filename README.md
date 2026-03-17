# 🍔 FoodRush BD — Food Delivery System

A C++ console-based food delivery management system for Bangladesh, featuring intelligent route optimization, priority-based order scheduling, and real-time delivery tracking across major cities.

---

## 📋 Table of Contents

- [Overview](#overview)
- [Features](#features)
- [Algorithms Used](#algorithms-used)
- [City Network](#city-network)
- [Project Structure](#project-structure)
- [Getting Started](#getting-started)
- [Usage](#usage)
- [Data Persistence](#data-persistence)
- [Performance Analysis](#performance-analysis)

---

## Overview

FoodRush BD is a terminal-based food delivery platform that simulates a real-world delivery network across 12 cities in Bangladesh. It supports user registration, restaurant browsing, order placement with priority tiers, and an admin panel for managing deliveries with optimized routing.

---

## ✨ Features

### 👤 User Side
- Register & login with credential validation
- Browse restaurants by city
- Place orders with Normal, Priority, or VIP tiers
- View order history and cancel pending orders
- Dynamic delivery fee calculation based on distance and priority

### 🛠️ Admin Side
- View and manage delivery van status
- Start and complete deliveries with optimized routes
- Add new restaurants dynamically
- View business analytics (revenue, average delivery time, top customers)
- Run and view algorithm performance benchmarks

### 🚐 Delivery System
- 3 delivery vans based in Dhaka, Chittagong, and Mymensingh
- Capacity and weight constraints per van
- Real-time estimated delivery time calculation

---

## 🧠 Algorithms Used

| Algorithm | Purpose | Complexity |
|-----------|---------|------------|
| **TSP (Held-Karp DP)** | Optimal multi-stop delivery route | O(2ⁿ · n²) |
| **0/1 Knapsack (3D DP)** | Priority-based order selection per van | O(n · C · W) |
| **Greedy Sort** | Fallback order prioritization benchmark | O(n log n) |
| **Dijkstra (implicit)** | Shortest path between cities via graph | O(E log V) |

### TSP — Travelling Salesman Problem
Used to find the optimal delivery route when a van serves multiple cities. Implemented with bitmask dynamic programming (Held-Karp algorithm).

### Knapsack
Selects the best set of orders for each van given its item capacity and weight capacity, maximizing total priority score.

### Greedy Heuristic
Used as a performance comparison baseline — sorts orders by priority and measures runtime against the Knapsack DP approach.

---

## 🗺️ City Network

The system models a weighted graph of 12 cities in Bangladesh:

```
Dhaka ── Narayanganj (20 km) ── Comilla (80 km) ── Chittagong (150 km)
  │                                                        │
Gazipur (30 km)                                    Cox's Bazar (150 km)
  │
Mymensingh (90 km) ── Tangail (70 km)
  │
Kishoreganj (60 km) ── Narsingdi (50 km) ── Narayanganj (30 km)

Comilla ── Sylhet (200 km) ── Moulvibazar (40 km)
```

---

## 📁 Project Structure

```
FoodRush-BD/
├── main.cpp                  # Full source code
├── user_data.csv             # Persisted user accounts (auto-generated)
├── active_orders.csv         # Active orders state (auto-generated)
├── completed_orders.csv      # Completed order history (auto-generated)
├── tsp_performance.csv       # TSP benchmark results (auto-generated)
├── knapsack_performance.csv  # Knapsack benchmark results (auto-generated)
├── greedy_performance.csv    # Greedy benchmark results (auto-generated)
└── README.md
```

---

## 🚀 Getting Started

### Prerequisites
- C++17 or later
- g++ compiler (GCC 7+)

### Compilation

```bash
g++ -std=c++17 -O2 -o foodrush main.cpp
```

### Run

```bash
./foodrush
```

---

## 🖥️ Usage

### Main Menu
```
1. Register       — Create a new user account
2. Login          — Log in as an existing user
3. Admin Login    — Access the admin panel (default: admin / admin123)
4. Exit           — Save all data and quit
```

### User Menu
```
1. Place Order        — Browse restaurants, select items, choose priority
2. View Order History — See all past and active orders
3. Cancel Order       — Cancel a pending order
4. Exit
```

### Admin Menu
```
1. View Delivery Status     — See all vans and their current loads
2. Start Delivery           — Dispatch a van with optimized TSP route
3. Complete Delivery        — Mark a delivery as done
4. View Analytics           — Revenue, delivery stats, top customers
5. Add Restaurant           — Register a new restaurant
6. View All Pending Orders  — See all unassigned orders
7. Performance Analysis     — View algorithm runtime data
8. Run Performance Tests    — Benchmark TSP, Knapsack, and Greedy
9. Exit
```

### Priority Tiers

| Tier | Multiplier | Description |
|------|-----------|-------------|
| Normal (1) | 1.0× | Standard delivery |
| Priority (2) | 1.5× | Faster processing |
| VIP (3) | 2.0× | Highest priority, dispatched first |

---

## 💾 Data Persistence

All data is saved to CSV files automatically on exit:

| File | Contents |
|------|----------|
| `user_data.csv` | Usernames, passwords, addresses, order counts |
| `active_orders.csv` | All currently pending/in-transit orders |
| `completed_orders.csv` | Historical record of delivered orders |

Data is reloaded on the next launch, ensuring session continuity.

---

## 📊 Performance Analysis

The system includes built-in benchmarking for its core algorithms. After running **"Run Performance Tests"** from the admin menu, results are exported to CSV files suitable for Excel or Python analysis.

**TSP** is tested across 2–6 city inputs. **Knapsack** and **Greedy** are tested across order sets of size 2–10. Runtime is measured in microseconds using `std::chrono::high_resolution_clock`.

---

## 🏪 Pre-loaded Restaurants

| City | Restaurants |
|------|-------------|
| Dhaka | Burger King, Pizza Hut, KFC, Nando's |
| Chittagong | Pizza Hut, KFC |
| Gazipur | KFC |
| Narayanganj | Star Kabab |
| Sylhet | Panshi Restaurant |
| Mymensingh | Mama Halim |
| Comilla | Rosmalai House |
| Cox's Bazar | Sea Pearl |

---

## 📌 Notes

- Passwords must be at least 6 characters long.
- Usernames must be alphanumeric (underscores allowed).
- Maximum order weight is **20 kg** per order.
- Each van has a capacity of **10 items** and **50 kg** weight limit.

---

## 👨‍💻 Author

Built as an academic project demonstrating applied data structures and algorithm design in a real-world logistics simulation.
