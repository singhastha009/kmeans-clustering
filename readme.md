# 📊 K-Means Clustering in C

A C-based implementation of the K-Means Clustering algorithm designed to cluster n-dimensional data into k groups. This project was developed to understand the internal workings of unsupervised learning by implementing core logic manually in C.

---

## 🧠 Features

- Implements K-Means from scratch in C
- Allows user-defined number of clusters (`k`) and data points
- Calculates Euclidean distance between points and centroids
- Iteratively updates centroids until convergence
- Reads input data from file or user input
- Prints final clusters and centroids

---

## 🛠 Technologies

- Language: C  
- Build Tools: `gcc` / `make`
- Optional: Standard C libraries only (e.g., `stdio.h`, `stdlib.h`, `math.h`)

---

## 🚀 How to Compile and Run

### 1. Compile using GCC:

gcc kmeans.c -o kmeans -lm
./kmeans

