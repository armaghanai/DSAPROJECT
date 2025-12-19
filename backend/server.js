const express = require('express');
const cors = require('cors');
const { spawn } = require('child_process');
const path = require('path');

const app = express();
const PORT = 3000;

// Middleware
app.use(cors());
app.use(express.json());
app.use(express.static('public'));

// Path to C++ executable
const CPP_EXECUTABLE = path.join(__dirname, '..', 'build', 'main.exe');

let cppProcess = null;
let requestQueue = [];
let currentRequestId = 0;
let pendingRequests = new Map();

function initializeCppProcess() {
    console.log('🚀 Starting persistent C++ search engine process...');
    
    cppProcess = spawn(CPP_EXECUTABLE, [], {
        stdio: ['pipe', 'pipe', 'pipe']
    });
    
    let buffer = '';
    
    cppProcess.stdout.on('data', (data) => {
        buffer += data.toString();
        
        // Process complete JSON lines
        let lines = buffer.split('\n');
        buffer = lines.pop(); // Keep incomplete line in buffer
        
        for (let line of lines) {
            if (!line.trim()) continue;
            
            if (!line.trim().startsWith('{')) {
                console.log('[C++ Output]', line);
                continue;
            }
            
            try {
                const response = JSON.parse(line);
                
                // Check if this is the ready signal
                if (response.status === 'ready') {
                    console.log('✓ C++ search engine is READY!');
                    console.log('  Initialization complete - accepting queries\n');
                    return;
                }
                
                // Find and resolve the pending request
                const requestId = response.request_id || 0;
                const resolver = pendingRequests.get(requestId);
                
                if (resolver) {
                    resolver.resolve(response);
                    pendingRequests.delete(requestId);
                }
            } catch (err) {
                // Not JSON - probably debug output, just log it
                console.log('[C++]', line);
            }
        }
    });
    
    cppProcess.stderr.on('data', (data) => {
        // Print C++ debug output (progress, logs, etc.)
        const lines = data.toString().split('\n');
        for (const line of lines) {
            if (line.trim()) {
                console.log('[C++]', line);
            }
        }
    });
    
    // Handle process exit
    cppProcess.on('close', (code) => {
        console.error(`❌ C++ process exited with code ${code}`);
        cppProcess = null;
        
        // Reject all pending requests
        for (let [id, resolver] of pendingRequests) {
            resolver.reject(new Error('C++ process crashed'));
        }
        pendingRequests.clear();
        
        // Auto-restart after 1 second
        console.log('Restarting C++ process in 1 second...');
        setTimeout(initializeCppProcess, 1000);
    });
    
    cppProcess.on('error', (err) => {
        console.error('Failed to start C++ process:', err);
    });
}

function sendCppRequest(requestData) {
    return new Promise((resolve, reject) => {
        if (!cppProcess) {
            reject(new Error('C++ process not running'));
            return;
        }
        
        const requestId = currentRequestId++;
        requestData.request_id = requestId;
        
        // Store resolver
        pendingRequests.set(requestId, { resolve, reject });
        
        // Send JSON request to C++ stdin
        const jsonLine = JSON.stringify(requestData) + '\n';
        cppProcess.stdin.write(jsonLine);
        
        setTimeout(() => {
            if (pendingRequests.has(requestId)) {
                pendingRequests.delete(requestId);
                reject(new Error('Request timeout'));
            }
        }, 30000);
    });
}

let cachedStats = {
    totalDocs: 45000,
    vocabSize: 125000,
    avgDocLength: 250
};

// API Routes

app.get('/api/stats', (req, res) => {
    res.json(cachedStats);
});

app.post('/api/search', async (req, res) => {
    try {
        const { query, mode = 'hybrid', top_k = 10 } = req.body;
        
        if (!query || query.trim() === '') {
            return res.status(400).json({ error: 'Query is required' });
        }
        
        console.log(`🔍 Search request: "${query}" (${mode}, top_k=${top_k})`);
        
        const startTime = Date.now();
        
        const response = await sendCppRequest({
            type: 'search',
            query: query,
            mode: mode,
            top_k: top_k
        });
        
        const searchTime = Date.now() - startTime;
        
        console.log(`  ✓ Completed in ${searchTime}ms (found ${response.results?.length || 0} results)`);
        
        res.json({
            query: response.query,
            mode: response.mode,
            results: response.results || [],
            search_time_ms: searchTime,
            total_results: response.total_results || 0
        });
        
    } catch (error) {
        console.error('❌ Search error:', error);
        res.status(500).json({ 
            error: 'Search failed', 
            message: error.message 
        });
    }
});

app.get('/api/autocomplete', async (req, res) => {
    try {
        const { prefix, limit = 10 } = req.query;
        
        if (!prefix || prefix.length < 2) {
            return res.json({ suggestions: [] });
        }
        
        const response = await sendCppRequest({
            type: 'autocomplete',
            prefix: prefix,
            limit: parseInt(limit)
        });
        
        res.json(response);
        
    } catch (error) {
        console.error('Autocomplete error:', error);
        res.json({ suggestions: [] });
    }
});

// Health check
app.get('/health', (req, res) => {
    res.json({ 
        status: cppProcess ? 'ok' : 'error',
        cpp_process_running: cppProcess !== null,
        timestamp: new Date().toISOString() 
    });
});

// Graceful shutdown
process.on('SIGINT', () => {
    console.log('\n🛑 Shutting down...');
    
    if (cppProcess) {
        // Send exit command to C++ process
        sendCppRequest({ type: 'exit' }).catch(() => {});
        
        setTimeout(() => {
            cppProcess.kill();
            process.exit(0);
        }, 1000);
    } else {
        process.exit(0);
    }
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
    console.log(`  GET  /health             - Server health check`);
    console.log(`\nFrontend: http://localhost:${PORT}`);
    console.log(`========================================\n`);
    
    // ✅ Initialize persistent C++ process
    initializeCppProcess();
});