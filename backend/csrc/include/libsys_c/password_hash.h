#ifndef LIBSYS_C_PASSWORD_HASH_H
#define LIBSYS_C_PASSWORD_HASH_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * 纯 C 密码哈希模块 (bcrypt via libxcrypt)
 *
 * 约定:
 *  - password_hash 返回的字符串由 malloc 分配, 调用方负责 free()
 *    失败返回 NULL.
 *  - 所有函数线程安全 (内部使用 thread-local crypt_data 缓冲).
 */

/* 生成 bcrypt 哈希, cost=12, 前缀 $2b$.
 * 返回值: malloc 出的字符串 (调用方 free), 或 NULL 失败. */
char *password_hash(const char *plain);

/* 校验明文与已有哈希是否匹配.
 * 返回: 1 匹配, 0 不匹配或参数非法. */
int password_verify(const char *hash, const char *plain);

/* 判断字符串是否为 bcrypt 哈希 ($2a$/$2b$/$2y$ 前缀).
 * 返回: 1 是, 0 否. */
int password_is_bcrypt(const char *hash);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* LIBSYS_C_PASSWORD_HASH_H */
