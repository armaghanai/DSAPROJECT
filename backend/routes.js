import express from "express";
import { runCpp } from "./runCpp.js";

const router = express.Router();

router.get("/suggest", async (req, res) => {
  const data = await runCpp("suggest", req.query.q);
  res.json(data);
});

router.get("/search", async (req, res) => {
  const data = await runCpp("search", req.query.q);
  res.json(data);
});

router.get("/doc/:id", (req, res) => {
  res.sendFile(`../docs/${req.params.id}.txt`, { root: "." });
});

export default router;
