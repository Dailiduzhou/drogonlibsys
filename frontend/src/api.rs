use gloo_net::http::Request;
use serde::{Deserialize, Serialize};

use crate::auth::{clear_tokens, load_access_token, save_tokens};

pub const API_BASE: &str = match option_env!("LIBSYS_API_BASE") {
    Some(base) => base,
    None => "/api",
};

#[derive(Debug, Clone, Serialize, Deserialize, PartialEq)]
pub struct ApiResponse<T> {
    #[serde(default)]
    pub code: i32,
    #[serde(default)]
    pub msg: String,
    #[serde(default = "Option::default")]
    pub data: Option<T>,
}

#[derive(Debug, Clone, Serialize, Deserialize, Default, PartialEq)]
pub struct TokenPair {
    #[serde(rename = "accessToken", default)]
    pub access_token: String,
    #[serde(rename = "refreshToken", default)]
    pub refresh_token: String,
    #[serde(rename = "accessExpiresAt", default)]
    pub access_expires_at: i64,
    #[serde(rename = "refreshExpiresAt", default)]
    pub refresh_expires_at: i64,
}

#[derive(Debug, Clone, Serialize, Deserialize, Default, PartialEq)]
pub struct Book {
    #[serde(default)]
    pub id: i64,
    #[serde(default)]
    pub title: String,
    #[serde(default)]
    pub author: String,
    #[serde(default)]
    pub description: String,
    #[serde(rename = "coverKey", default)]
    pub cover_key: String,
    #[serde(default)]
    pub stock: i32,
    #[serde(rename = "createdAt", default)]
    pub created_at: String,
    #[serde(rename = "updatedAt", default)]
    pub updated_at: String,
}

#[derive(Debug, Clone, Serialize, Deserialize, Default, PartialEq)]
pub struct BookCreate {
    pub title: String,
    pub author: String,
    #[serde(default, skip_serializing_if = "String::is_empty")]
    pub description: String,
    #[serde(rename = "coverKey", default, skip_serializing_if = "String::is_empty")]
    pub cover_key: String,
    #[serde(default)]
    pub stock: i32,
}

#[derive(Debug, Clone, Serialize, Deserialize, Default, PartialEq)]
pub struct BookUpdate {
    #[serde(default, skip_serializing_if = "String::is_empty")]
    pub title: String,
    #[serde(default, skip_serializing_if = "String::is_empty")]
    pub author: String,
    #[serde(default, skip_serializing_if = "String::is_empty")]
    pub description: String,
    #[serde(rename = "coverKey", default, skip_serializing_if = "String::is_empty")]
    pub cover_key: String,
    #[serde(default)]
    pub stock: i32,
}

#[derive(Debug, Clone, Serialize, Deserialize, Default, PartialEq)]
pub struct LoanRecord {
    #[serde(default)]
    pub id: i64,
    #[serde(rename = "bookId", default)]
    pub book_id: i64,
    #[serde(rename = "userId", default)]
    pub user_id: i64,
    #[serde(default)]
    pub status: String,
    #[serde(rename = "borrowedAt", default)]
    pub borrowed_at: String,
    #[serde(rename = "returnedAt", default)]
    pub returned_at: Option<String>,
    #[serde(rename = "createdAt", default)]
    pub created_at: String,
    #[serde(rename = "updatedAt", default)]
    pub updated_at: String,
}

#[derive(Debug, Clone, Serialize, Deserialize, Default, PartialEq)]
pub struct LoanCreate {
    #[serde(rename = "bookId")]
    pub book_id: i64,
    #[serde(rename = "userId")]
    pub user_id: i64,
    #[serde(default = "default_status")]
    pub status: String,
    #[serde(rename = "borrowedAt")]
    pub borrowed_at: String,
    #[serde(rename = "returnedAt", default, skip_serializing_if = "Option::is_none")]
    pub returned_at: Option<String>,
}

fn default_status() -> String {
    "borrowed".to_string()
}

#[derive(Debug, Clone, Serialize, Deserialize, Default, PartialEq)]
pub struct LoanUpdate {
    #[serde(rename = "bookId")]
    pub book_id: i64,
    #[serde(rename = "userId")]
    pub user_id: i64,
    pub status: String,
    #[serde(rename = "borrowedAt")]
    pub borrowed_at: String,
    #[serde(rename = "returnedAt", default, skip_serializing_if = "Option::is_none")]
    pub returned_at: Option<String>,
}

#[derive(Debug, Clone, Serialize, Deserialize, Default, PartialEq)]
pub struct CoverUploadResult {
    #[serde(rename = "coverKey", default)]
    pub cover_key: String,
    #[serde(rename = "coverUrl", default)]
    pub cover_url: String,
}

#[derive(Debug, Clone, Serialize, Deserialize, Default, PartialEq)]
pub struct IdOnly {
    #[serde(default)]
    pub id: i64,
}

#[derive(Debug, Clone)]
pub enum ApiError {
    Network(String),
    Message(String),
}

impl std::fmt::Display for ApiError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            ApiError::Network(m) => write!(f, "network error: {m}"),
            ApiError::Message(m) => write!(f, "{m}"),
        }
    }
}

impl From<gloo_net::Error> for ApiError {
    fn from(e: gloo_net::Error) -> Self {
        ApiError::Network(e.to_string())
    }
}

pub type ApiResult<T> = Result<T, ApiError>;

async fn parse_envelope<T: for<'de> Deserialize<'de>>(
    resp: gloo_net::http::Response,
) -> ApiResult<T> {
    let status = resp.status();
    let text = resp
        .text()
        .await
        .map_err(|e| ApiError::Network(e.to_string()))?;
    if !(200..300).contains(&status) {
        if let Ok(env) = serde_json::from_str::<ApiResponse<serde_json::Value>>(&text) {
            return Err(ApiError::Message(if env.msg.is_empty() {
                format!("HTTP {status}")
            } else {
                env.msg
            }));
        }
        return Err(ApiError::Message(format!("HTTP {status}: {text}")));
    }
    let env: ApiResponse<T> = serde_json::from_str(&text)
        .map_err(|e| ApiError::Message(format!("parse: {e} / body={text}")))?;
    if env.code != 0 {
        return Err(ApiError::Message(if env.msg.is_empty() {
            format!("code {}", env.code)
        } else {
            env.msg
        }));
    }
    env.data
        .ok_or_else(|| ApiError::Message("empty response data".into()))
}

async fn parse_empty(resp: gloo_net::http::Response) -> ApiResult<()> {
    let status = resp.status();
    let text = resp
        .text()
        .await
        .map_err(|e| ApiError::Network(e.to_string()))?;
    if !(200..300).contains(&status) {
        if let Ok(env) = serde_json::from_str::<ApiResponse<serde_json::Value>>(&text) {
            return Err(ApiError::Message(if env.msg.is_empty() {
                format!("HTTP {status}")
            } else {
                env.msg
            }));
        }
        return Err(ApiError::Message(format!("HTTP {status}: {text}")));
    }
    if text.is_empty() {
        return Ok(());
    }
    let env: ApiResponse<serde_json::Value> = serde_json::from_str(&text)
        .map_err(|e| ApiError::Message(format!("parse: {e} / body={text}")))?;
    if env.code != 0 {
        return Err(ApiError::Message(if env.msg.is_empty() {
            format!("code {}", env.code)
        } else {
            env.msg
        }));
    }
    Ok(())
}

fn with_auth(req: gloo_net::http::RequestBuilder) -> gloo_net::http::RequestBuilder {
    if let Some(token) = load_access_token() {
        if !token.is_empty() {
            return req.header("Authorization", &format!("Bearer {token}"));
        }
    }
    req
}

pub async fn login(username: &str, password: &str) -> ApiResult<TokenPair> {
    let body = serde_json::json!({ "username": username, "password": password });
    let resp = Request::post(&format!("{}/auth/login", API_BASE))
        .header("Content-Type", "application/json")
        .body(body.to_string())?
        .send()
        .await?;
    let pair: TokenPair = parse_envelope(resp).await?;
    save_tokens(&pair);
    Ok(pair)
}

pub async fn register(username: &str, password: &str) -> ApiResult<TokenPair> {
    let body = serde_json::json!({ "username": username, "password": password });
    let resp = Request::post(&format!("{}/auth/register", API_BASE))
        .header("Content-Type", "application/json")
        .body(body.to_string())?
        .send()
        .await?;
    let pair: TokenPair = parse_envelope(resp).await?;
    save_tokens(&pair);
    Ok(pair)
}

/// Admin-only user creation: calls /api/auth/register but does NOT overwrite
/// the currently logged-in admin's stored tokens (unlike `register`, which
/// persists the new session for self-registration/login flows).
pub async fn admin_create_user(username: &str, password: &str) -> ApiResult<TokenPair> {
    let body = serde_json::json!({ "username": username, "password": password });
    let resp = Request::post(&format!("{}/auth/register", API_BASE))
        .header("Content-Type", "application/json")
        .body(body.to_string())?
        .send()
        .await?;
    parse_envelope(resp).await
}

pub async fn refresh(refresh_token: &str) -> ApiResult<TokenPair> {
    let body = serde_json::json!({ "refreshToken": refresh_token });
    let resp = Request::post(&format!("{}/auth/refresh", API_BASE))
        .header("Content-Type", "application/json")
        .body(body.to_string())?
        .send()
        .await?;
    let pair: TokenPair = parse_envelope(resp).await?;
    save_tokens(&pair);
    Ok(pair)
}

pub async fn logout() -> ApiResult<()> {
    let resp = with_auth(Request::post(&format!("{}/auth/logout", API_BASE)))
        .send()
        .await?;
    let out = parse_empty(resp).await;
    clear_tokens();
    out
}

pub async fn list_books(offset: i32, limit: i32) -> ApiResult<Vec<Book>> {
    let url = format!("{}/books?offset={offset}&limit={limit}", API_BASE);
    let resp = Request::get(&url).send().await?;
    parse_envelope(resp).await
}

pub async fn get_book(id: i64) -> ApiResult<Book> {
    let url = format!("{}/books/{id}", API_BASE);
    let resp = Request::get(&url).send().await?;
    parse_envelope(resp).await
}

pub async fn create_book(input: BookCreate) -> ApiResult<IdOnly> {
    let body = serde_json::to_string(&input)
        .map_err(|e| ApiError::Message(format!("encode: {e}")))?;
    let resp = with_auth(Request::post(&format!("{}/books", API_BASE)))
        .header("Content-Type", "application/json")
        .body(body)?
        .send()
        .await?;
    parse_envelope(resp).await
}

pub async fn update_book(id: i64, input: BookUpdate) -> ApiResult<()> {
    let body = serde_json::to_string(&input)
        .map_err(|e| ApiError::Message(format!("encode: {e}")))?;
    let resp = with_auth(Request::put(&format!("{}/books/{id}", API_BASE)))
        .header("Content-Type", "application/json")
        .body(body)?
        .send()
        .await?;
    parse_empty(resp).await
}

pub async fn delete_book(id: i64) -> ApiResult<()> {
    let resp = with_auth(Request::delete(&format!("{}/books/{id}", API_BASE)))
        .send()
        .await?;
    parse_empty(resp).await
}

pub async fn borrow_book(id: i64) -> ApiResult<LoanRecord> {
    let resp = with_auth(Request::post(&format!("{}/books/{id}/borrow", API_BASE)))
        .send()
        .await?;
    parse_envelope(resp).await
}

pub async fn return_book(id: i64) -> ApiResult<LoanRecord> {
    let resp = with_auth(Request::post(&format!("{}/books/{id}/return", API_BASE)))
        .send()
        .await?;
    parse_envelope(resp).await
}

pub async fn list_loans(offset: i32, limit: i32) -> ApiResult<Vec<LoanRecord>> {
    let url = format!("{}/loans?offset={offset}&limit={limit}", API_BASE);
    let resp = with_auth(Request::get(&url)).send().await?;
    parse_envelope(resp).await
}

pub async fn create_loan(input: LoanCreate) -> ApiResult<IdOnly> {
    let body = serde_json::to_string(&input)
        .map_err(|e| ApiError::Message(format!("encode: {e}")))?;
    let resp = with_auth(Request::post(&format!("{}/loans", API_BASE)))
        .header("Content-Type", "application/json")
        .body(body)?
        .send()
        .await?;
    parse_envelope(resp).await
}

pub async fn get_loan(id: i64) -> ApiResult<LoanRecord> {
    let resp = with_auth(Request::get(&format!("{}/loans/{id}", API_BASE)))
        .send()
        .await?;
    parse_envelope(resp).await
}

pub async fn update_loan(id: i64, input: LoanUpdate) -> ApiResult<()> {
    let body = serde_json::to_string(&input)
        .map_err(|e| ApiError::Message(format!("encode: {e}")))?;
    let resp = with_auth(Request::put(&format!("{}/loans/{id}", API_BASE)))
        .header("Content-Type", "application/json")
        .body(body)?
        .send()
        .await?;
    parse_empty(resp).await
}

pub async fn delete_loan(id: i64) -> ApiResult<()> {
    let resp = with_auth(Request::delete(&format!("{}/loans/{id}", API_BASE)))
        .send()
        .await?;
    parse_empty(resp).await
}

pub async fn search_books(q: &str, offset: i32, limit: i32) -> ApiResult<Vec<Book>> {
    let encoded = js_sys::encode_uri_component(q).as_string().unwrap_or_default();
    let url = format!(
        "{}/search?q={encoded}&offset={offset}&limit={limit}",
        API_BASE
    );
    let resp = Request::get(&url).send().await?;
    parse_envelope(resp).await
}

pub async fn upload_cover(
    book_id: i64,
    filename: &str,
    content_type: Option<&str>,
    bytes: Vec<u8>,
) -> ApiResult<CoverUploadResult> {
    use js_sys::Uint8Array;
    use wasm_bindgen::JsValue;

    let array = Uint8Array::new_with_length(bytes.len() as u32);
    array.copy_from(&bytes);
    let parts = js_sys::Array::new();
    parts.push(&JsValue::from(array));

    let blob = if let Some(ct) = content_type {
        let opts = web_sys::BlobPropertyBag::new();
        opts.set_type(ct);
        web_sys::Blob::new_with_u8_array_sequence_and_options(&parts, &opts)
    } else {
        web_sys::Blob::new_with_u8_array_sequence(&parts)
    }
    .map_err(|e| ApiError::Message(format!("Blob: {e:?}")))?;

    let form = web_sys::FormData::new()
        .map_err(|e| ApiError::Message(format!("FormData: {e:?}")))?;
    form.append_with_blob_and_filename("file", &blob, filename)
        .map_err(|e| ApiError::Message(format!("append: {e:?}")))?;

    let url = format!("{}/books/{book_id}/cover", API_BASE);
    let mut req = Request::post(&url);
    if let Some(token) = load_access_token() {
        if !token.is_empty() {
            req = req.header("Authorization", &format!("Bearer {token}"));
        }
    }
    let resp = req
        .body(form)
        .map_err(|e| ApiError::Network(e.to_string()))?
        .send()
        .await?;
    parse_envelope(resp).await
}
