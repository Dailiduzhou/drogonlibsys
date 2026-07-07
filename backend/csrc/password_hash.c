#include "libsys_c/password_hash.h"

#include <crypt.h>
#include <stdlib.h>
#include <string.h>

/* bcrypt cost / 前缀常量 */
#define KBcryptPrefix "$2b$"
#define KBcryptCost 12
#define KMaxPlainLen 512

/* thread-local crypt_data 缓冲, 避免每次调 crypt_rn 分配栈空间 */
static _Thread_local struct crypt_data tls_crypt_data;

/* 判断 hash 是否为 bcrypt 前缀 ($2a$/$2b$/$2y$) */
int password_is_bcrypt(const char *hash) {
  if (hash == NULL) {
    return 0;
  }
  if (strncmp(hash, "$2a$", 4) == 0 || strncmp(hash, "$2b$", 4) == 0 ||
      strncmp(hash, "$2y$", 4) == 0) {
    return 1;
  }
  return 0;
}

/* 生成 bcrypt salt setting 串 (cost=12, $2b$ 前缀) */
static int gen_setting(char *out, size_t out_len) {
  /* CRYPT_GENSALT_OUTPUT_SIZE = 192, 我们的缓冲应当 >= 192 */
  if (out_len < CRYPT_GENSALT_OUTPUT_SIZE) {
    return 0;
  }
  char *result =
      crypt_gensalt_rn(KBcryptPrefix, KBcryptCost, NULL, 0, out, (int)out_len);
  if (result == NULL || result[0] == '\0') {
    return 0;
  }
  return 1;
}

char *password_hash(const char *plain) {
  if (plain == NULL) {
    return NULL;
  }
  size_t plain_len = strlen(plain);
  if (plain_len == 0 || plain_len >= KMaxPlainLen) {
    return NULL;
  }

  char setting[CRYPT_GENSALT_OUTPUT_SIZE];
  if (!gen_setting(setting, sizeof(setting))) {
    return NULL;
  }

  memset(&tls_crypt_data, 0, sizeof(tls_crypt_data));
  char *hashed =
      crypt_rn(plain, setting, &tls_crypt_data, (int)sizeof(tls_crypt_data));
  if (hashed == NULL || hashed[0] == '*') {
    return NULL;
  }

  /* 复制到 malloc 缓冲, 调用方负责 free */
  size_t hashed_len = strlen(hashed);
  char *out = (char *)malloc(hashed_len + 1);
  if (out == NULL) {
    return NULL;
  }
  memcpy(out, hashed, hashed_len + 1);
  return out;
}

int password_verify(const char *hash, const char *plain) {
  if (hash == NULL || plain == NULL) {
    return 0;
  }
  if (!password_is_bcrypt(hash)) {
    return 0;
  }
  size_t plain_len = strlen(plain);
  if (plain_len == 0 || plain_len >= KMaxPlainLen) {
    return 0;
  }

  memset(&tls_crypt_data, 0, sizeof(tls_crypt_data));
  char *hashed =
      crypt_rn(plain, hash, &tls_crypt_data, (int)sizeof(tls_crypt_data));
  if (hashed == NULL || hashed[0] == '*') {
    return 0;
  }
  /* 常时比较, 避免旁路时序差异 (strlen 相同即可比较) */
  size_t hlen_a = strlen(hashed);
  size_t hlen_b = strlen(hash);
  if (hlen_a != hlen_b) {
    return 0;
  }
  /* 借用 CRYPTO_memcmp 风格: 简单实现为对每一位都进行 XOR 累积 */
  volatile unsigned char diff = 0;
  for (size_t i = 0; i < hlen_b; ++i) {
    diff |= (unsigned char)(hashed[i] ^ hash[i]);
  }
  return diff == 0 ? 1 : 0;
}
