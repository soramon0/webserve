#include "parser.hpp"
#include <cstdlib>

ssize_t Parser::handleRoot(DirectiveCtx &ctx) {
  const Token &dir = previous();

  if (!consume(Directive::WORD, "expected path after `root` directive."))
    return -1;

  ctx.shared->root = previous().lexeme;

  return expectDirectiveArgsCount(dir);
}

ssize_t Parser::handleIndex(DirectiveCtx &ctx) {
  const Token &dir = previous();

  while (consume(Directive::WORD)) {
    ctx.shared->index.push_back(previous().lexeme);
  }

  if (ctx.shared->index.size() == 0) {
    reportParseError(previous(),
                     "at least one argument is required in `index` directive.");
    return -1;
  }

  return expectEnd(dir, Directive::SEMICOLON) ? 0 : -1;
}

ssize_t Parser::handleAutoIndex(DirectiveCtx &ctx) {
  const Token &dir = previous();

  if (!consume(Directive::WORD,
               "expected `on` or `off` after `autoindex` directive."))
    return -1;

  const Token &token = previous();
  if (token.lexeme != "on" && token.lexeme != "off") {
    reportParseError(token, "value must be `on` or `off`.");
    return -1;
  }

  ctx.shared->withAutoIndex(token.lexeme == "on");

  return expectDirectiveArgsCount(dir);
}

ssize_t Parser::handleErrorPage(DirectiveCtx &ctx) {
  const Token &dir = previous();

  std::vector<int> codes;
  while (check(Directive::WORD)) {
    const Token &t = peek();
    if (t.lexeme.find_first_not_of("0123456789") != std::string::npos)
      break;

    int code = std::atoi(t.lexeme.c_str());
    if (code < 300 || code > 599) {
      reportParseError(t, "value must be between 300 and 599.");
      return -1;
    }
    codes.push_back(code);
    advance();
  }

  if (codes.size() == 0) {
    reportParseError(peek(), "code is required in `error_page` directive.");
    return -1;
  }

  if (!consume(Directive::WORD,
               "expected path after code in `error_page` directive.")) {
    return -1;
  }

  std::string path = previous().lexeme;
  for (size_t i = 0; i < codes.size(); i++) {
    ctx.shared->withErrorPage(codes[i], path);
  }

  return expectDirectiveArgsCount(dir);
}

ssize_t Parser::handleClientMaxBodySize(DirectiveCtx &ctx) {
  const Token &dir = previous();

  if (!consume(Directive::WORD,
               "expected size after `client_max_body_size` directive."))
    return -1;

  if (previous().lexeme.find_first_not_of("0123456789") != std::string::npos) {
    reportParseError(previous(), "size must be a valid number.");
    return -1;
  }

  long long size = std::atoll(previous().lexeme.c_str());
  if (size <= 0) {
    reportParseError(previous(), "size must be greater than 0.");
    return -1;
  }

  ctx.shared->client_max_body_size = size;

  return expectDirectiveArgsCount(dir);
}

ssize_t Parser::handleMimeTypes(DirectiveCtx &ctx) {
  const Token &dir = previous();

  if (!consume(Directive::LBRACE, "expected '{' after `types` directive."))
    return -1;

  while (!check(Directive::RBRACE) && !atEnd()) {
    if (check(Directive::SEMICOLON)) {
      advance();
      continue;
    }

    if (!consume(Directive::WORD,
                 "expected MIME type or '}' inside `types` block."))
      return -1;

    std::string mime = previous().lexeme;
    bool has_ext = false;
    while (check(Directive::WORD)) {
      advance();
      ctx.shared->withMimetype(previous().lexeme, mime);
      has_ext = true;
    }

    if (!has_ext) {
      reportParseError(previous(),
                       "expected at least one extension after MIME type in "
                       "`types` block.");
      return -1;
    }

    if (!consume(Directive::SEMICOLON,
                 "expected ';' after MIME extensions in `types` block."))
      return -1;
  }

  return expectEnd(dir, Directive::RBRACE) ? 0 : -1;
}

ssize_t Parser::handleUploadStore(DirectiveCtx &ctx) {
  const Token &dir = previous();

  if (!consume(Directive::WORD,
               "expected path after `upload_store` directive."))
    return -1;

  ctx.shared->upload_store = previous().lexeme;

  return expectDirectiveArgsCount(dir);
}

ssize_t Parser::handleAccessLogPath(DirectiveCtx &ctx) {
  const Token &dir = previous();

  if (!consume(Directive::WORD, "expected path after `access_log` directive."))
    return -1;

  ctx.shared->access_log = previous().lexeme;

  return expectDirectiveArgsCount(dir);
}

ssize_t Parser::handleCgiPass(DirectiveCtx &ctx) {
  const Token &dir = previous();

  if (!consume(Directive::WORD,
               "expected extension after `cgi_pass` directive."))
    return -1;

  std::string ext = previous().lexeme;
  if (ext.empty()) {
    reportParseError(previous(), "extension can't be empty");
    return -1;
  }

  if (ext.size() <= 1) {
    reportParseError(previous(), "invalid extension.");
    return -1;
  }

  if (ext[0] != '.') {
    reportParseError(previous(), "extension must start with `.`.");
    return -1;
  }

  size_t i = 1;
  while (ext[i]) {
    ext[i] =
        static_cast<char>(std::tolower(static_cast<unsigned char>(ext[i])));
    if (!std::isalpha(ext[i]) && !std::isdigit(ext[i])) {
      reportParseError(previous(), "extension must be alphanumeric.");
      return -1;
    }
    i++;
  }

  if (!consume(Directive::WORD,
               "expected interpreter path after extension in `cgi_pass` "
               "directive."))
    return -1;

  ctx.shared->withCgi(ext, previous().lexeme);

  return expectDirectiveArgsCount(dir);
}

ssize_t Parser::handleMethods(DirectiveCtx &ctx) {
  const Token &dir = previous();

  if (this->ctx.back() != CTX_LOCATION) {
    reportParseError(dir, "`methods` directive is not allowed here.");
    return -1;
  }

  static std::map<std::string, int> methods;
  if (methods.empty()) {
    methods["GET"] = 1;
    methods["POST"] = 1;
    methods["DELETE"] = 1;
  }

  ctx.loc->methods.clear();
  while (check(Directive::WORD)) {
    const Token &t = peek();
    if (methods.find(t.lexeme) == methods.end()) {
      reportParseError(t, "unsupported http method.");
      return -1;
    }

    ctx.loc->methods.push_back(t.lexeme);
    advance();
  }

  if (ctx.loc->methods.size() == 0) {
    reportParseError(peek(), "http method is required in `methods` directive.");
    return -1;
  }

  return expectDirectiveArgsCount(dir);
}

ssize_t Parser::handleListen(DirectiveCtx &ctx) {
  const Token &dir = previous();

  if (this->ctx.back() != CTX_SERVER) {
    reportParseError(dir, "`listen` directive is not allowed here.");
    return -1;
  }

  if (!consume(Directive::WORD, "expected host:port after `listen` directive."))
    return -1;

  std::string raw = previous().lexeme;
  std::string host = "0.0.0.0";
  int port = 8080;

  size_t colonPos = raw.find(':');
  if (colonPos != std::string::npos) {
    // Format: host:port
    host = raw.substr(0, colonPos);
    std::string portStr = raw.substr(colonPos + 1);
    port = std::atoi(portStr.c_str());
  } else {
    // Format could be just "8080" (port) or "localhost" (host)
    if (raw.find_first_not_of("0123456789") == std::string::npos) {
      port = std::atoi(raw.c_str());
    } else {
      host = raw;
    }
  }

  if (port <= 0 || port > 65535) {
    reportParseError(previous(), "invalid port in `listen` directive.");
    return -1;
  }

  ctx.server->interface = host;
  ctx.server->port = port;

  return expectDirectiveArgsCount(dir);
}

ssize_t Parser::handleReturn(DirectiveCtx &ctx) {
  const Token &dir = previous();

  if (this->ctx.back() != CTX_SERVER && this->ctx.back() != CTX_LOCATION) {
    reportParseError(dir, "`return` directive is not allowed here.");
    return -1;
  }

  if (!consume(Directive::WORD,
               "expected [code?] [uri] after `return` directive."))
    return -1;

  int code = 0;
  std::string uri;
  std::string raw = previous().lexeme;
  if (raw.find_first_not_of("0123456789") == std::string::npos) {
    code = std::atoi(raw.c_str());
    if (code < 300 || code >= 400) {
      reportParseError(previous(), "invalid return code.");
      return -1;
    }
  } else {
    uri = raw;
    code = 302;
  }

  if (uri.empty()) {
    if (!consume(Directive::WORD,
                 "expected [uri] after code in `return` directive."))
      return -1;
    uri = previous().lexeme;
  }

  if (this->ctx.back() == CTX_SERVER) {
    ctx.server->withRedirect(code, uri);
  }
  if (this->ctx.back() == CTX_LOCATION) {
    ctx.loc->withRedirect(code, uri);
  }

  return expectDirectiveArgsCount(dir);
}

DirectiveCtx DirectiveCtx::withServer(Server *srv) {
  DirectiveCtx ctx;
  ctx.server = srv;
  ctx.loc = NULL;
  ctx.shared = srv->shared_config;
  return ctx;
}

DirectiveCtx DirectiveCtx::withLocation(Location *loc) {
  DirectiveCtx ctx;
  ctx.server = NULL;
  ctx.loc = loc;
  ctx.shared = loc->shared_config;
  return ctx;
}

DirectiveCtx DirectiveCtx::withSharedConfig(SharedConfig *cfg) {
  DirectiveCtx ctx;
  ctx.server = NULL;
  ctx.loc = NULL;
  ctx.shared = cfg;
  return ctx;
}
