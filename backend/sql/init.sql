CREATE EXTENSION IF NOT EXISTS pg_trgm;

CREATE TABLE IF NOT EXISTS users (
    id          BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    username    VARCHAR(64)  NOT NULL UNIQUE,
    password    VARCHAR(255) NOT NULL,
    role        VARCHAR(16)  NOT NULL DEFAULT 'user',
    created_at  TIMESTAMPTZ  NOT NULL DEFAULT now(),
    updated_at  TIMESTAMPTZ  NOT NULL DEFAULT now()
);

CREATE TABLE IF NOT EXISTS books (
    id          BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    title       VARCHAR(255) NOT NULL,
    author      VARCHAR(128) NOT NULL,
    description TEXT,
    cover_key   VARCHAR(255),
    stock       INTEGER      NOT NULL DEFAULT 0,
    created_at  TIMESTAMPTZ  NOT NULL DEFAULT now(),
    updated_at  TIMESTAMPTZ  NOT NULL DEFAULT now()
);

CREATE TABLE IF NOT EXISTS loan_records (
    id          BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    book_id     BIGINT       NOT NULL REFERENCES books(id) ON DELETE CASCADE,
    user_id     BIGINT       NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    status      VARCHAR(16)  NOT NULL DEFAULT 'borrowed',
    borrowed_at TIMESTAMPTZ  NOT NULL DEFAULT now(),
    returned_at TIMESTAMPTZ,
    created_at  TIMESTAMPTZ  NOT NULL DEFAULT now(),
    updated_at  TIMESTAMPTZ  NOT NULL DEFAULT now(),
    CONSTRAINT chk_loan_records_status
        CHECK (status IN ('borrowed', 'returned'))
);

CREATE INDEX IF NOT EXISTS idx_books_title_trgm  ON books USING gin (title gin_trgm_ops);
CREATE INDEX IF NOT EXISTS idx_books_author_trgm ON books USING gin (author gin_trgm_ops);

CREATE INDEX IF NOT EXISTS idx_books_search_trgm ON books 
    USING gin ((coalesce(title, '') || ' ' || coalesce(author, '') || ' ' || coalesce(description, '')) gin_trgm_ops);

CREATE INDEX IF NOT EXISTS idx_loan_records_book_id ON loan_records (book_id);
CREATE INDEX IF NOT EXISTS idx_loan_records_user_id ON loan_records (user_id);
CREATE INDEX IF NOT EXISTS idx_loan_records_status ON loan_records (status);
CREATE UNIQUE INDEX IF NOT EXISTS uniq_loan_records_active_book_user
    ON loan_records (book_id, user_id)
    WHERE status = 'borrowed';

CREATE OR REPLACE FUNCTION set_updated_at()
RETURNS TRIGGER AS $$
BEGIN
    NEW.updated_at = now();
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

DROP TRIGGER IF EXISTS trg_users_updated_at ON users;
CREATE TRIGGER trg_users_updated_at BEFORE UPDATE ON users
    FOR EACH ROW EXECUTE FUNCTION set_updated_at();

DROP TRIGGER IF EXISTS trg_books_updated_at ON books;
CREATE TRIGGER trg_books_updated_at BEFORE UPDATE ON books
    FOR EACH ROW EXECUTE FUNCTION set_updated_at();

DROP TRIGGER IF EXISTS trg_loan_records_updated_at ON loan_records;
CREATE TRIGGER trg_loan_records_updated_at BEFORE UPDATE ON loan_records
    FOR EACH ROW EXECUTE FUNCTION set_updated_at();

INSERT INTO users (username, password, role)
VALUES ('admin', '$2y$12$6sTLncSjnX3iFNJ2zv0NhO/.OSdEMRUqiAc/mb5ebc34rhutRADDC', 'admin')
ON CONFLICT (username) DO NOTHING;
