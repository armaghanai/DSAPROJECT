// server.js - Node.js backend for C++ search engine
const express = require('express');
const cors = require('cors');
const { spawn } = require('child_process');
const path = require('path');

const app = express();
const PORT = 3000;

// Middleware
app.use(cors());
app.use(express.json());
app.use(express.static('public')); // Serve HTML frontend

// Path to your compiled C++ executable
// Updated to point to build/main.exe (CMake output)
const CPP_EXECUTABLE = path.join(__dirname, '..', 'build', 'main.exe'); // For Windows with CMake
// For Linux/Mac: path.join(__dirname, '..', 'build', 'main')

// Cache for stats (loaded once)
let cachedStats = {
    totalDocs: 0,
    vocabSize: 0,
    avgDocLength: 0
};

// Initialize stats on startup
function initializeStats() {
    // You can run your C++ program with a special flag to get stats
    // For now, using placeholder values
    cachedStats = {
        totalDocs: 45000,
        vocabSize: 125000,
        avgDocLength: 250
    };
    console.log('Stats initialized:', cachedStats);
}

// Call C++ search engine
function callCppSearch(query, mode, topK) {
    return new Promise((resolve, reject) => {
        // Arguments to pass to C++ program
        const args = ['--search', query, '--mode', mode, '--topk', topK.toString()];
        
        const cpp = spawn(CPP_EXECUTABLE, args);
        
        let stdout = '';
        let stderr = '';
        
        cpp.stdout.on('data', (data) => {
            stdout += data.toString();
        });
        
        cpp.stderr.on('data', (data) => {
            stderr += data.toString();
        });
        
        cpp.on('close', (code) => {
            if (code !== 0) {
                console.error('C++ Error:', stderr);
                reject(new Error('Search engine failed'));
                return;
            }
            
            try {
                // Parse JSON output from C++
                const results = JSON.parse(stdout);
                resolve(results);
            } catch (err) {
                console.error('Parse error:', err);
                reject(new Error('Failed to parse results'));
            }
        });
        
        cpp.on('error', (err) => {
            console.error('Spawn error:', err);
            reject(err);
        });
    });
}

// Call C++ autocomplete
function callCppAutocomplete(prefix, limit = 10) {
    return new Promise((resolve, reject) => {
        const args = ['--autocomplete', prefix, '--limit', limit.toString()];
        
        const cpp = spawn(CPP_EXECUTABLE, args);
        
        let stdout = '';
        
        cpp.stdout.on('data', (data) => {
            stdout += data.toString();
        });
        
        cpp.on('close', (code) => {
            if (code !== 0) {
                resolve({ suggestions: [] });
                return;
            }
            
            try {
                const results = JSON.parse(stdout);
                resolve(results);
            } catch (err) {
                resolve({ suggestions: [] });
            }
        });
    });
}

// API Routes

// GET /api/stats - Get corpus statistics
app.get('/api/stats', (req, res) => {
    res.json(cachedStats);
});

// POST /api/search - Perform search
app.post('/api/search', async (req, res) => {
    try {
        const { query, mode = 'hybrid', top_k = 10 } = req.body;
        
        if (!query || query.trim() === '') {
            return res.status(400).json({ error: 'Query is required' });
        }
        
        console.log(`Search request: "${query}" (${mode})`);
        
        const startTime = Date.now();
        const results = await callCppSearch(query, mode, top_k);
        const searchTime = Date.now() - startTime;
        
        res.json({
            query,
            mode,
            results: results.results || [],
            search_time_ms: searchTime,
            total_results: results.total_results || 0
        });
        
    } catch (error) {
        console.error('Search error:', error);
        res.status(500).json({ 
            error: 'Search failed', 
            message: error.message 
        });
    }
});

// GET /api/autocomplete - Get autocomplete suggestions
app.get('/api/autocomplete', async (req, res) => {
    try {
        const { prefix, limit = 10 } = req.query;
        
        if (!prefix || prefix.length < 2) {
            return res.json({ suggestions: [] });
        }
        
        const results = await callCppAutocomplete(prefix, parseInt(limit));
        res.json(results);
        
    } catch (error) {
        console.error('Autocomplete error:', error);
        res.json({ suggestions: [] });
    }
});

// Health check
app.get('/health', (req, res) => {
    res.json({ status: 'ok', timestamp: new Date().toISOString() });
});

// Start server
app.listen(PORT, () => {
    console.log(`\n========================================`);
    console.log(`Server running on http://localhost:${PORT}`);
    console.log(`========================================\n`);
    console.log(`API Endpoints:`);
    console.log(`  GET  /api/stats          - Corpus statistics`);
    console.log(`  POST /api/search         - Perform search`);
    console.log(`  GET  /api/autocomplete   - Get suggestions`);
    console.log(`\nFrontend: http://localhost:${PORT}`);
    console.log(`========================================\n`);
    
    initializeStats();
});