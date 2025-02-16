**Operating Systems Project - IST 2021/22**

This repository contains the implementation of the **TecnicoFS**, a simplified in-user-space file system developed for the **Operating Systems** course at Instituto Superior Técnico (IST). The project focuses on **file system operations, concurrency handling, and synchronization techniques**.

## **Project Overview**

### **1. File System Implementation**
- Developed a **custom file system API** inspired by POSIX.
- Implemented **file operations**: `open`, `close`, `write`, `read`.
- Created **i-node structures** for file metadata management.
- Managed a **fixed-size block storage** system.

### **2. Multi-Block File Storage**
- Removed the **one-block-per-file** limitation.
- Implemented **direct and indirect block referencing** for large files.
- Optimized block allocation using **linked index tables**.

### **3. External File Copying**
- Developed `tfs_copy_to_external_fs()`, allowing file export from **TecnicoFS** to the system's file system.
- Ensured safe handling of missing or existing destination files.

### **4. Thread-Safe Concurrency Support**
- Introduced **pthreads** to enable multi-threaded clients.
- Implemented fine-grained synchronization using **mutexes** and **read-write locks**.
- Prevented data corruption using **thread-safe operations**.

### **5. Blocking Destruction Mechanism**
- Implemented `tfs_destroy_after_all_closed()`, blocking destruction until all files are closed.
- Utilized **condition variables** to handle thread synchronization efficiently.

## **Installation & Setup**

1. **Clone the Repository**
   ```bash
   git clone https://github.com/yourusername/operating-systems-project.git
   cd operating-systems-project
   ```

2. **Build the Project**
   ```bash
   make
   ```

3. **Run Tests**
   ```bash
   ./tecnicofs testfile
   ```

## **Results Summary**
- **Extended file system capabilities** with multi-block storage.
- **Implemented file export feature** for external system access.
- **Enabled concurrent file access** using multi-threading.
- **Developed a blocking file system destruction mechanism**.



