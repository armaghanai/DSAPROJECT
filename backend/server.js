import express from "express";
import { runCpp } from "./runCpp.js";

const app = express();

app.get("/api/search", async (req, res) => {
  const q = req.query.q;
  try {
    const data = await runCpp(["search", q]);
    res.json(data);
  } catch (err) {
    res.status(500).json({ error: err.toString() });
  }
});

app.get("/api/suggest", async (req, res) => {
  const q = req.query.q;
  try {
    const data = await runCpp(["suggest", q]);
    res.json(data);
  } catch (err) {
    res.status(500).json({ error: err.toString() });
  }
});

app.listen(3000, () => console.log("Server running at http://localhost:3000"));
