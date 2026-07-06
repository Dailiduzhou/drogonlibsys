-- 图书管理系统 PostgreSQL 初始化脚本
-- 包含: 用户表、图书表、tsvector 全文检索字段、GIN 倒排索引

-- 用户表 (鉴权用)
CREATE TABLE IF NOT EXISTS users (
    id          BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    username    VARCHAR(64)  NOT NULL UNIQUE,
    password    VARCHAR(255) NOT NULL,          -- 存储 bcrypt 哈希
    role        VARCHAR(16)  NOT NULL DEFAULT 'user',
    created_at  TIMESTAMPTZ  NOT NULL DEFAULT now(),
    updated_at  TIMESTAMPTZ  NOT NULL DEFAULT now()
);

-- 图书表
CREATE TABLE IF NOT EXISTS books (
    id          BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    title       VARCHAR(255) NOT NULL,
    author      VARCHAR(128) NOT NULL,
    description TEXT,
    cover_key   VARCHAR(255),                   -- MinIO Object Key
    stock       INTEGER      NOT NULL DEFAULT 0,
    created_at  TIMESTAMPTZ  NOT NULL DEFAULT now(),
    updated_at  TIMESTAMPTZ  NOT NULL DEFAULT now()
);

-- 全文检索: 基于 title / author / description 构造 tsvector
-- title 权重 A, author 权重 B, description 权重 C
ALTER TABLE books
    ADD COLUMN IF NOT EXISTS tsv tsvector
    GENERATED ALWAYS AS (
        setweight(to_tsvector('simple', coalesce(title, '')),       'A') ||
        setweight(to_tsvector('simple', coalesce(author, '')),      'B') ||
        setweight(to_tsvector('simple', coalesce(description, '')), 'C')
    ) STORED;

-- GIN 倒排索引, 加速 tsquery 检索
CREATE INDEX IF NOT EXISTS idx_books_tsv ON books USING gin (tsv);

-- 普通索引: 按 title / author 模糊匹配的补充
CREATE INDEX IF NOT EXISTS idx_books_title  ON books (title);
CREATE INDEX IF NOT EXISTS idx_books_author ON books (author);

-- 更新时间触发器
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
