-- Files table - stores current state of files in the system
CREATE TABLE IF NOT EXISTS files (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    relative_path TEXT NOT NULL UNIQUE,  -- Relative path of the file, serves as unique identifier
    content_hash TEXT NOT NULL,          -- Hash of file contents only (excluding metadata)
    size_bytes INTEGER NOT NULL,         -- File size in bytes
    last_read_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,    -- Last time file was read
    last_write_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,   -- Last time file was modified
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP       -- When the file was first tracked
);

-- Indexes for optimizing queries
CREATE INDEX idx_files_path ON files(relative_path);         -- Fast lookup by path
CREATE INDEX idx_files_content_hash ON files(content_hash);  -- Fast lookup by content hash

-- Trigger to update last read timestamp
CREATE TRIGGER update_last_read
AFTER SELECT ON files
BEGIN
    UPDATE files SET last_read_at = CURRENT_TIMESTAMP 
    WHERE id = OLD.id;
END;

-- Trigger to update last write timestamp when content changes
CREATE TRIGGER update_last_write
AFTER UPDATE OF content_hash, size_bytes ON files
BEGIN
    UPDATE files SET last_write_at = CURRENT_TIMESTAMP 
    WHERE id = NEW.id;
END;

-- Versions table - tracks historical versions of files
CREATE TABLE IF NOT EXISTS versions (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    file_id INTEGER NOT NULL,            -- Reference to files table
    hash TEXT NOT NULL,                  -- Hash of this version's content
    commit_id TEXT NOT NULL,             -- Reference to commits table
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (file_id) REFERENCES files(id),
    FOREIGN KEY (commit_id) REFERENCES commits(id)
);

-- Commits table - stores commit metadata
CREATE TABLE IF NOT EXISTS commits (
    id TEXT PRIMARY KEY,                 -- Commit hash
    message TEXT NOT NULL,               -- Commit message
    author TEXT NOT NULL,                -- Author of the commit
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);