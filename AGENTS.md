# 图书管理系统 (Library Management System) 需求规格与技术架构

## 1. 技术栈选型
* **Web 框架**: Drogon (基于 C++14/17 的高性能异步 Web 框架)
* **构建系统与包管理**: Xmake (基于 Lua 的现代跨平台 C/C++ 构建工具，结合 Xrepo 统一管理第三方依赖)
* **关系型数据库**: PostgreSQL (主业务数据存储、倒排索引与全文搜索)
* **缓存与中间件**: Redis (热点数据缓存、JWT 黑名单管控)
* **对象存储 (OSS)**: MinIO (图书封面等静态资源存储)
* **鉴权机制**: JWT (JSON Web Token) 双 Token 机制 (Access Token + Refresh Token)
* **并发控制**: Singleflight (防缓存击穿机制)
* **Git版本控制**: 进行版本控制，使用Pull Request添加新功能

---

## 2. 核心功能模块设计

### 2.1 鉴权与安全模块 (Auth & Security)
* **双 Token 机制**:
  * 用户登录成功后，下发短有效期的 `Access Token` 和长有效期的 `Refresh Token`。
  * `Access Token` 过期后，客户端使用 `Refresh Token` 换取新的 Token 对。
* **中间件拦截 (Drogon Filter)**:
  * 基于 Drogon 的 `HttpFilter` 实现 JWT 鉴权中间件，对受保护的路由进行拦截与 Token 校验。
* **JWT 黑名单 (Blacklist)**:
  * 用户登出、修改密码或使用 `Refresh Token` 刷新后，将旧的/失效的 Token 的 `jti` (JWT ID) 存入 Redis 黑名单，并设置 TTL（与 Token 剩余有效期一致），防止 Token 被恶意重放。

### 2.2 图书管理与 OSS 存储模块 (Book & Storage)
* **基础 CRUD**: 提供图书的增删改查 RESTful API。
* **封面文件上传**:
  * 接收客户端上传的 `multipart/form-data` 图书封面图片。
  * 后端通过兼容 S3 的 C++ SDK 将图片直传至 MinIO 存储桶 (Bucket)。
  * 数据库仅保存 MinIO 返回的图片对象 URL 或 Object Key。

### 2.3 全文搜索模块 (Full-Text Search)
* **倒排索引构建**:
  * 利用 PostgreSQL 的全文检索能力 (Full Text Search)。
  * 针对图书的 `description` (描述)、`title` (标题)、`author` (作者) 等字段 使用pg_trgm进行全文搜索
  * 在数据库层面建立 GIN (Generalized Inverted Index) 倒排索引，加速文本检索。
* **搜索 API**:
  * 支持三元组匹配与相似度排序，客户端传入关键词，后端使用 `pg_trgm` 进行高效检索。

### 2.4 高并发与性能优化 (Performance)
* **Singleflight 机制**:
  * 针对高频的图书详情查询、热门搜索等接口实现 Singleflight 模式（请求合并）。
  * 当多个并发请求同时查询同一个未命中缓存的图书 Key 时，只允许一个请求打到 PostgreSQL 数据库，其余请求阻塞等待，获取结果后共享返回，有效防止**缓存击穿** (Cache Stampede)。
* **Redis 缓存层**:
  * 配合 Singleflight，将高频读取的图书元数据和搜索结果缓存在 Redis 中，减轻 PG 数据库压力。

### 2.5 工程化与构建 (Engineering)
* **Xmake 配置与编译**:
  * 编写 `xmake.lua` 作为工程的唯一构建脚本。
  * 通过 Xmake 内置的包管理器自动化拉取和集成 `drogon`, `jwt-cpp`, `hiredis`, `libpqxx` 等核心依赖，解决 C++ 依赖管理痛点。
  * 配置不同的构建模式 (Debug/Release) 以及对应的编译优化参数。

---

## 3. 架构请求链路示意

1. **Client** -> 发起 HTTP 请求 (携带 JWT)
2. **Drogon Filter (鉴权中间件)** -> 解析 JWT -> 查 Redis 黑名单 -> 验证通过
3. **Controller (业务层)** -> 
   * *若是上传请求*: 走 OSS Service 将流写入 MinIO
   * *若是查询请求*: 走 Singleflight 组件 -> 查 Redis 缓存 -> 未命中查 PostgreSQL (命中 GIN 倒排索引) -> 回写 Redis -> 响应 Client
