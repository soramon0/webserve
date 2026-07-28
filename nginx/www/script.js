const UPLOAD_DIR = "/uploads/";

// --- 6. List + delete files -------------------------------------------
// Parses the server's autoindex HTML to get filenames. If your autoindex
// markup differs, tweak the selector below (it just grabs <a href="...">).
async function loadFiles() {
  const listEl = document.getElementById("file-list");
  listEl.innerHTML = "<li>loading…</li>";

  try {
    const res = await fetch(UPLOAD_DIR);
    if (!res.ok) throw new Error(`GET ${UPLOAD_DIR} -> ${res.status}`);
    const html = await res.text();

    const doc = new DOMParser().parseFromString(html, "text/html");
    const links = [...doc.querySelectorAll("a")]
      .map(a => a.getAttribute("href"))
      .filter(href => href && !href.endsWith("/") && href !== "../");

    listEl.innerHTML = links.length === 0 ? "<li>(no files)</li>" : "";
    links.forEach(href => {
      const li = document.createElement("li");
      const displayName = href.split("/").filter(Boolean).pop(); // just for display
      li.innerHTML = `<span>${displayName}</span>`;
      const btn = document.createElement("button");
      btn.className = "delete-btn";
      btn.textContent = "Delete";
      btn.onclick = () => deleteFile(href); // use href AS-IS, don't rebuild it
      li.appendChild(btn);
      listEl.appendChild(li);
    });
  } catch (err) {
    listEl.innerHTML = `<li>error loading list: ${err.message}</li>`;
  }
}

async function deleteFile(href) {
  // href is already a valid path (absolute or relative) from the server — use it directly
  const path = href.startsWith("/") ? href : UPLOAD_DIR + href;
  try {
    const res = await fetch(path, { method: "DELETE" });
    console.log(`DELETE ${path} -> ${res.status}`);
    if (res.ok || res.status === 204) loadFiles();
    else alert(`Delete failed: ${res.status} ${res.statusText}`);
  } catch (err) {
    alert(`Delete request failed: ${err.message}`);
  }
}
// --- 4. Wrong method on a GET-only location -> expect 405 --------------
async function testWrongMethod(event) {
  event.preventDefault();
  const res = await fetch("/get-only/", { method: "POST" });
  alert(`POST /get-only/ -> ${res.status} ${res.statusText}`);
  return false;
}

// --- 4. Oversized body -> expect 413 -----------------------------------
async function testHugeBody(event) {
  event.preventDefault();
  const bigBody = "x".repeat(50 * 1024 * 1024); // 50MB, adjust vs your client_max_body_size
  const res = await fetch("/uploads/", { method: "POST", body: bigBody });
  alert(`POST oversized body -> ${res.status} ${res.statusText}`);
  return false;
}

document.addEventListener("DOMContentLoaded", loadFiles);
