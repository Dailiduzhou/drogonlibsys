// 密码哈希纯 C 模块单元测试
// 验证 password_hash / password_verify 行为, 不依赖 Drogon
#include <libsys_c/password_hash.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>

static int failures = 0;

static void check(int cond, const char *msg) {
    if (!cond) {
        ++failures;
        std::fprintf(stderr, "FAIL: %s\n", msg);
    } else {
        std::printf("ok  : %s\n", msg);
    }
}

int main() {
    // 1. is_bcrypt 前缀识别
    check(password_is_bcrypt("$2b$12$abcdef") == 1, "$2b$ 前缀识别");
    check(password_is_bcrypt("$2a$10$abcdef") == 1, "$2a$ 前缀识别");
    check(password_is_bcrypt("$2y$12$abcdef") == 1, "$2y$ 前缀识别");
    check(password_is_bcrypt("$3$abcdef") == 0, "非 bcrypt 前缀");
    check(password_is_bcrypt(NULL) == 0, "NULL 输入安全");

    // 2. hash 返回 $2b$12$ 前缀
    char *h1 = password_hash("password123");
    check(h1 != NULL, "hash 非空");
    if (h1) {
        check(strncmp(h1, "$2b$12$", 7) == 0, "hash 前缀 $2b$12$");
        check(strlen(h1) == 60, "bcrypt 哈希长度 60");

        // 3. 正确密码校验
        check(password_verify(h1, "password123") == 1, "正确密码校验通过");

        // 4. 错误密码校验
        check(password_verify(h1, "wrong") == 0, "错误密码校验失败");

        // 5. 不同明文生成不同哈希
        char *h2 = password_hash("password123");
        check(h2 != NULL && strcmp(h1, h2) != 0, "相同明文生成不同 salt 哈希");
        if (h2) {
            check(password_verify(h2, "password123") == 1,
                  "第二个哈希校验通过");
            free(h2);
        }
        free(h1);
    }

    // 6. 边界: 空明文
    check(password_hash("") == NULL, "空明文拒绝");

    // 7. 边界: NULL 明文
    check(password_hash(NULL) == NULL, "NULL 明文拒绝");

    // 8. 边界: 超长明文 (>= 512)
    char longbuf[600];
    memset(longbuf, 'a', sizeof(longbuf) - 1);
    longbuf[sizeof(longbuf) - 1] = '\0';
    check(password_hash(longbuf) == NULL, "超长明文拒绝");

    // 9. verify 边界
    check(password_verify(NULL, "x") == 0, "verify NULL hash 拒绝");
    check(password_verify("notabcrypt", "x") == 0, "verify 非 bcrypt 拒绝");
    check(password_verify("$2b$12$invalid", "") == 0,
          "verify 空明文拒绝");

    if (failures == 0) {
        std::printf("\nAll password_hash tests passed.\n");
        return 0;
    }
    std::printf("\n%d test(s) failed.\n", failures);
    return 1;
}
