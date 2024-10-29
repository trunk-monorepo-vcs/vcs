# Backend for VCS

## Database Design

### Files Table (files)
- id: INTEGER PRIMARY KEY
- relative_path: TEXT NOT NULL UNIQUE    // Relative path as unique identifier
- content_hash: TEXT NOT NULL           // Hash of file contents only
- size_bytes: INTEGER NOT NULL          // File size in bytes
- last_read_at: TIMESTAMP              // Last read timestamp
- last_write_at: TIMESTAMP             // Last write timestamp
- created_at: TIMESTAMP                // Creation timestamp

### Key Operations:

1. Update file hash: