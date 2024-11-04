-- File status tracking table
CREATE TABLE IF NOT EXISTS file_status (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    path TEXT NOT NULL UNIQUE,           -- File path relative to mount point
    hash TEXT NOT NULL,                  -- File content hash
    size INTEGER NOT NULL,               -- File size in bytes
    modified_time INTEGER NOT NULL,      -- Last modification timestamp
    sync_status INTEGER NOT NULL,        -- 0: unsynced, 1: synced, 2: conflict
    last_sync_time INTEGER,             -- Last successful sync timestamp
    version INTEGER DEFAULT 1,           -- File version number
    is_deleted INTEGER DEFAULT 0         -- Soft delete flag
);

-- File operation history table
CREATE TABLE IF NOT EXISTS file_history (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    file_id INTEGER NOT NULL,           -- References file_status.id
    operation TEXT NOT NULL,            -- READ, WRITE, DELETE
    timestamp INTEGER NOT NULL,         -- Operation timestamp
    size INTEGER,                       -- Size of operation (for READ/WRITE)
    offset INTEGER,                     -- Offset of operation (for READ/WRITE)
    FOREIGN KEY (file_id) REFERENCES file_status(id)
);

-- Create indexes for better query performance
CREATE INDEX IF NOT EXISTS idx_file_path ON file_status(path);
CREATE INDEX IF NOT EXISTS idx_file_sync ON file_status(sync_status);
CREATE INDEX IF NOT EXISTS idx_file_history ON file_history(file_id, timestamp); 