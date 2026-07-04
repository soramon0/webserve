#include "request_state.hpp"
#include "fsm.hpp"
#include "http/http_method.hpp"
#include "http/http_version.hpp"
#include "http/status_code.hpp"
#include "http_request.hpp"
#include "logger/log.hpp"
#include <cctype>
#include <cstddef>

State stateStart(Context &ctx) {
  Logger::debug("state: start");

  if (!std::isalpha(static_cast<unsigned char>(ctx.buf[ctx.offset]))) {
    ctx.fsm.setMalformed400("malformed request-line");
    return stateError(ctx);
  }
  return stateMethod;
}

State stateMethod(Context &ctx) {
  Logger::debug("state: method");
  size_t start = ctx.offset;

  int sp = 0;
  bool hasChar = false;
  while (ctx.offset < ctx.len) {
    if (ctx.buf[ctx.offset] == ' ') {
      sp = 1;
      ctx.offset++;
      break;
    }

    if (ctx.fsm.isCRLF(ctx.buf[ctx.offset])) {
      ctx.fsm.setMalformed400("missing http version");
      return stateError(ctx);
    }

    hasChar = std::isalpha(static_cast<unsigned char>(ctx.buf[ctx.offset]));
    if (!hasChar) {
      break;
    }
    ctx.offset++;
  }

  if (!hasChar && ctx.req->method_view.empty()) {
    ctx.fsm.setMalformed400("invalid request method");
    return stateError(ctx);
  }

  size_t size = ctx.offset - sp - start;
  if (!ctx.req->updateField(ctx.req->method_view, &ctx.buf[start], size)) {
    ctx.fsm.setMalformed(ctx.req->status.value(), ctx.req->error);
    return stateError(ctx);
  }

  if (!sp) {
    return stateMethod;
  }

  ctx.req->method = HttpMethod(ctx.req->method_view);
  if (ctx.req->method.isUnknown()) {
    ctx.fsm.setMalformed(HttpStatus::NOT_IMPLEMENTED, "method not implemented");
    return stateError(ctx);
  }

  if (!ctx.req->method.isSupported()) {
    ctx.fsm.setMalformed(HttpStatus::METHOD_NOT_ALLOWED, "method not allowed");
    return stateError(ctx);
  }

  return stateURI;
}

State stateURI(Context &ctx) {
  Logger::debug("state: URI");

  size_t start = ctx.offset;
  int sp = 0;
  while (ctx.offset < ctx.len) {
    if (ctx.buf[ctx.offset] == ' ') {
      sp = 1;
      ctx.offset++;
      break;
    }

    if (ctx.fsm.isCRLF(ctx.buf[ctx.offset])) {
      ctx.fsm.setMalformed400("missing http version");
      return stateError(ctx);
    }

    ctx.offset++;
  }

  size_t size = ctx.offset - sp - start;
  if (!ctx.req->updateField(ctx.req->uri, &ctx.buf[start], size)) {
    ctx.fsm.setMalformed(ctx.req->status.value(), ctx.req->error);
    return stateError(ctx);
  }

  if (sp && ctx.req->uri.empty()) {
    ctx.fsm.setMalformed400("uri is required");
    return stateError(ctx);
  }

  if (!sp) {
    return stateURI;
  }

  // Origin-from
  if (ctx.req->uri[0] == '/') {
    return stateVersion;
  }

  // Absolute-form
  size_t scheme_offset = 0;
  if (ctx.req->uri.startsWith("https://")) {
    scheme_offset = 8;
  } else if (ctx.req->uri.startsWith("http://")) {
    scheme_offset = 7;
  }

  if (scheme_offset > 0) {
    const char *domain = ctx.req->uri.data() + scheme_offset;
    size_t remaining_len = ctx.req->uri.length() - scheme_offset;

    // Reject missing domains (e.g., just "http://")
    if (remaining_len == 0) {
      ctx.fsm.setMalformed400("uri is invalid");
      return stateError(ctx);
    }

    const char *path =
        static_cast<const char *>(std::memchr(domain, '/', remaining_len));

    if (!path) {
      // e.g., "https://example.com"|"https://e" -> defaults to "/"
      ctx.req->uri = StringView("/", 1);
    } else {
      size_t new_len = (ctx.req->uri.data() + ctx.req->uri.length()) - path;
      ctx.req->uri = StringView(path, new_len);
    }

    return stateVersion;
  }

  ctx.fsm.setMalformed400("invalid uri in request-line");
  return stateError(ctx);
}

State stateVersion(Context &ctx) {
  Logger::debug("state: version");

  size_t start = ctx.offset;
  while (ctx.offset < ctx.len && !ctx.fsm.isCRLF(ctx.buf[ctx.offset])) {
    ctx.offset++;
  }

  size_t size = ctx.offset - start;
  if (!ctx.req->updateField(ctx.req->version_view, &ctx.buf[start], size)) {
    ctx.fsm.setMalformed(ctx.req->status.value(), ctx.req->error);
    return stateError(ctx);
  }

  if (ctx.offset == ctx.len) {
    return stateVersion;
  }

  if (ctx.req->version.isUnknown()) {
    if (ctx.req->version_view.empty()) {
      ctx.fsm.setMalformed400("http version is required");
      return stateError(ctx);
    }

    ctx.req->version = HttpVersion(ctx.req->version_view);
    if (ctx.req->version.isUnknown() || !ctx.req->version.isSupported()) {
      ctx.fsm.setMalformed400("http version is not supported");
      return stateError(ctx);
    }
  }

  if (!ctx.fsm.consumeCRLF(ctx.buf, ctx.len, ctx.offset)) {
    return stateVersion;
  }

  return stateHeaderKey;
}

State stateHeaderKey(Context &ctx) {
  Logger::debug("state: headerKey");

  // parse request header and save it in fsm.current_key
  size_t start = ctx.offset;
  if (ctx.fsm.isCRLF(ctx.buf[start])) {
    if (ctx.req->version == HttpVersion::V1_1) {
      const StringView *host = ctx.req->headers.get("host");
      if (!host || host->empty()) {
        ctx.fsm.setMalformed400();
        return stateError(ctx);
      }
    }
    if (!ctx.fsm.consumeCRLF(ctx.buf, ctx.len, ctx.offset)) {
      return stateHeaderKey;
    }
    // force done state until we implment body state
    return stateDone(ctx);
  }

  // header-field   = field-name ":" OWS field-value OWS
  // field-name     = token
  // token          = 1*tchar
  // tchar          = "!" / "#" / "$" / "%" / "&" / "'" / "*"
  //                / "+" / "-" / "." / "^" / "_" / "`" / "|" / "~"
  //                DIGIT / ALPHA
  while (ctx.offset < ctx.len && ctx.buf[ctx.offset] != ':') {
    if (!Headers::isValidKeyChar(ctx.buf[ctx.offset])) {
      ctx.fsm.setMalformed400();
      return stateError(ctx);
    }

    ctx.offset++;
  }

  size_t size = ctx.offset - start;
  if (!ctx.req->updateField(ctx.fsm.curr_header_key, &ctx.buf[start], size)) {
    ctx.fsm.setMalformed(ctx.req->status.value(), ctx.req->error);
    return stateError(ctx);
  }

  if (ctx.offset == ctx.len) {
    return stateHeaderKey;
  }

  if (ctx.fsm.curr_header_key.empty()) {
    ctx.fsm.setMalformed400("header key invalid");
    return stateError(ctx);
  }

  ctx.offset++;
  return stateHeaderValue;
}

// header-field   = field-name ":" OWS field-value OWS
// field-value    = *field-content
// field-content  = field-vchar [ 1*( SP / HTAB ) field-vchar ]
// field-vchar    = VCHAR / obs-text
State stateHeaderValue(Context &ctx) {
  Logger::debug("state: headerValue");
  StringView &key = ctx.fsm.curr_header_key;
  StringView &value = ctx.fsm.curr_header_value;

  if (ctx.fsm.isCRLF(ctx.buf[ctx.offset]) && key.empty() && value.empty()) {
    if (!ctx.fsm.consumeCRLF(ctx.buf, ctx.len, ctx.offset)) {
      return stateHeaderValue;
    }
    return stateHeaderKey;
  }

  size_t start = ctx.offset;
  while (ctx.offset < ctx.len && !ctx.fsm.isCRLF(ctx.buf[ctx.offset])) {
    ctx.offset++;
  }

  size_t size = ctx.offset - start;
  if (!ctx.req->updateField(value, &ctx.buf[start], size)) {
    ctx.fsm.setMalformed(ctx.req->status.value(), ctx.req->error);
    return stateError(ctx);
  }

  if (ctx.fsm.isCRLF(ctx.buf[ctx.offset])) {
    Headers::normalizeKey(const_cast<char *>(key.data()), key.length());
    if (!Headers::isValidKey(key)) {
      ctx.fsm.setMalformed400();
      return stateError(ctx);
    }

    Headers::normalizeValue(value);
    if (!Headers::isValidValue(value)) {
      ctx.fsm.setMalformed400();
      return stateError(ctx);
    }

    ctx.req->headers.set(key, value);
    key.clear();
    value.clear();

    if (!ctx.fsm.consumeCRLF(ctx.buf, ctx.len, ctx.offset)) {
      return stateHeaderValue;
    }
    return stateHeaderKey;
  }

  return stateHeaderValue;
}

State stateBody(Context &ctx) {
  Logger::debug("state: body");
  if (ctx.req->method != HttpMethod::POST) {
    return stateDone(ctx);
  }
  // parse body
  return stateDone(ctx);
}

State stateDone(Context &ctx) {
  Logger::debug("state: done");
  // skip to end of buffer
  ctx.offset = ctx.len;
  ctx.fsm.setDone();
  return stateDone;
}

State stateError(Context &ctx) {
  Logger::debug("state: error");
  // skip to end of buffer
  ctx.offset = ctx.len;
  if (ctx.fsm.status.isPending()) {
    ctx.fsm.status = FSMStatus::MALFORMED;
  }
  return stateError;
}
