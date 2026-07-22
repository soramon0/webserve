# !/usr/bin/env python3
# -*- coding: utf-8 -*-

import warnings
warnings.filterwarnings("ignore", category=DeprecationWarning)

import os
import cgi
import uuid
import json
import http.cookies

STATIC_DIR = os.environ.get("SCRIPT_ROOT", "")
# BASE         = os.path.join(STATIC_DIR, "var/www/cgi-bin")
BASE = os.path.join(os.path.dirname(os.environ["SCRIPT_FILENAME"]), "")
LOGIN_PAGE   = os.path.join(BASE, "session_login.html")
HOME_PAGE    = os.path.join(BASE, "session_home.html")
WELCOME_PAGE = os.path.join(BASE, "session_welcome.html")
SESSION_DB   = os.path.join(BASE, "session_database.json")

def load_db():
    if not os.path.exists(SESSION_DB):
        return {}
    try:
        with open(SESSION_DB, "r") as f:
            return json.load(f)
    except:
        return {}

def save_db(db):
    with open(SESSION_DB, "w") as f:
        json.dump(db, f)

def set_cookie(value, expires=None):
    c = http.cookies.SimpleCookie()
    c["session_id"] = value
    c["session_id"]["path"] = "/"
    c["session_id"]["httponly"] = True
    c["session_id"]["samesite"] = "Strict"
    if expires:
        c["session_id"]["expires"] = expires
    print(c.output())

def get_cookie():
    raw = os.environ.get("HTTP_COOKIE", "")
    c = http.cookies.SimpleCookie(raw)
    obj = c.get("session_id")
    return obj.value if obj else None

def send_page(path, username=None):
    print("Content-Type: text/html\n")
    with open(path, "r") as f:
        content = f.read()
    if username:
        content = content.replace("{{username}}", username)
    print(content, end="")

def main():
    form = cgi.FieldStorage()
    db   = load_db()

    # ── LOGOUT ────────────────────────────────────────────────────────────────
    if "logout" in form:
        # Only expire the cookie — keep the DB entry so returning login works
        set_cookie("", expires="Thu, 01 Jan 1970 00:00:00 GMT")
        send_page(LOGIN_PAGE)

    # ── LOGIN ─────────────────────────────────────────────────────────────────
    elif "username" in form:
        username = form.getvalue("username").strip()

        existing_id = None
        for sid, data in db.items():
            if data.get("username") == username:
                existing_id = sid
                break

        if existing_id:
            # Known user → welcome back
            set_cookie(existing_id)
            send_page(WELCOME_PAGE, username=username)
        else:
            # Brand new user → home page
            session_id = str(uuid.uuid4())
            db[session_id] = {"username": username}
            save_db(db)
            set_cookie(session_id)
            send_page(HOME_PAGE, username=username)

    # ── REFRESH ───────────────────────────────────────────────────────────────
    else:
        session_id = get_cookie()

        if session_id and session_id in db:
            username = db[session_id]["username"]
            send_page(WELCOME_PAGE, username=username)
        else:
            send_page(LOGIN_PAGE)

if __name__ == "__main__":
    main()

