const input = document.getElementById("search");
const suggestionsDiv = document.getElementById("suggestions");
const resultsDiv = document.getElementById("results");
const timeDiv = document.getElementById("time");

// ===== Autocomplete on typing =====
input.addEventListener("input", async () => {
  const q = input.value.trim();
  if (!q) {
    suggestionsDiv.innerHTML = "";
    return;
  }

  try {
    const res = await fetch(`/suggest?q=${encodeURIComponent(q)}`);
    const data = await res.json();

    suggestionsDiv.innerHTML = "";
    data.data.forEach(s => {
      const div = document.createElement("div");
      div.className = "suggestion";
      div.innerText = s;
      div.onclick = () => search(s);
      suggestionsDiv.appendChild(div);
    });
  } catch (err) {
    console.error("Autocomplete error:", err);
  }
});

// ===== Search function =====
async function search(q) {
  const query = q.trim();
  if (!query) return;

  try {
    const res = await fetch(`/search?q=${encodeURIComponent(query)}`);
    const data = await res.json();

    // Clear previous suggestions and results
    suggestionsDiv.innerHTML = "";
    resultsDiv.innerHTML = "";

    // Show query time
    timeDiv.innerText = `Query time: ${data.time_ms.toFixed(2)} ms`;

    // Display results
    if (!data.results || data.results.length === 0) {
      resultsDiv.innerText = "No results found.";
      return;
    }

    data.results.forEach(r => {
      const div = document.createElement("div");
      // Using docId as title for now; replace with r.title if available from backend
      div.innerHTML = `<a href="/doc/${r.docId}" target="_blank">Document ID: ${r.docId} (Score: ${r.score.toFixed(2)})</a>`;
      resultsDiv.appendChild(div);
    });
  } catch (err) {
    console.error("Search error:", err);
  }
}

// ===== Trigger search on Enter key =====
input.addEventListener("keydown", e => {
  if (e.key === "Enter") search(input.value);
});
