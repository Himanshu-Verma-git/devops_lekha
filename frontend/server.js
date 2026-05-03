const express = require('express');
const session = require('express-session');
const bodyParser = require('body-parser');
const { Pool } = require('pg');

const app = express();
const port = process.env.PORT || 80;

// Database Connection
const pool = new Pool({
  connectionString: process.env.POSTGRES_URL || 'postgresql://lekhaadmin:change_me_in_prod123@localhost/lekhadb',
  ssl: (process.env.POSTGRES_URL && process.env.POSTGRES_URL.includes('rds.amazonaws.com')) ? { rejectUnauthorized: false } : false
});

// Log connection errors
pool.on('error', (err, client) => {
  console.error('Unexpected error on idle client', err);
});

// Middleware
app.set('view engine', 'ejs');
app.use(bodyParser.urlencoded({ extended: true }));
app.use(session({
    secret: 'lekha-secret-key-12345',
    resave: false,
    saveUninitialized: true
}));

// Initialize Database Schemas
async function initDB() {
    try {
        await pool.query(`
            CREATE TABLE IF NOT EXISTS users (
                id SERIAL PRIMARY KEY,
                username VARCHAR(255) UNIQUE NOT NULL,
                device_id VARCHAR(255) UNIQUE NOT NULL
            );
        `);
        await pool.query(`
            CREATE TABLE IF NOT EXISTS records (
                id SERIAL PRIMARY KEY,
                user_id INTEGER REFERENCES users(id) ON DELETE CASCADE,
                timestamp VARCHAR(255) NOT NULL,
                transcription TEXT,
                tags VARCHAR(255),
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
            );
        `);
        console.log("Database initialized successfully.");
    } catch (e) {
        console.error("Database initialization error:", e);
    }
}
initDB();

// Middleware to check authentication
function isAuthenticated(req, res, next) {
    if (req.session.user) return next();
    res.redirect('/login');
}

// Routes
app.get('/', (req, res) => {
    if (req.session.user) {
        res.redirect('/dashboard');
    } else {
        res.redirect('/login');
    }
});

app.get('/login', (req, res) => {
    res.render('login', { error: null });
});

app.post('/login', async (req, res) => {
    const { username } = req.body;
    try {
        const result = await pool.query('SELECT * FROM users WHERE username = $1', [username]);
        if (result.rows.length > 0) {
            req.session.user = result.rows[0];
            res.redirect('/dashboard');
        } else {
            res.render('login', { error: 'Invalid Username. Try signing up.' });
        }
    } catch (e) {
        console.error("Login error:", e);
        res.render('login', { error: 'Database error occurred.' });
    }
});

app.get('/signup', (req, res) => {
    res.render('signup', { error: null, success: null });
});

app.post('/signup', async (req, res) => {
    const { username, device_id } = req.body;
    try {
        await pool.query('INSERT INTO users (username, device_id) VALUES ($1, $2)', [username, device_id]);
        res.render('signup', { error: null, success: 'Signup successful! You can now log in.' });
    } catch (e) {
        console.error("Signup error:", e);
        if (e.code === '23505') {
            res.render('signup', { error: 'Username or Device ID already exists.', success: null });
        } else {
            res.render('signup', { error: 'An error occurred during signup.', success: null });
        }
    }
});

app.get('/logout', (req, res) => {
    req.session.destroy();
    res.redirect('/login');
});

app.get('/dashboard', isAuthenticated, async (req, res) => {
    try {
        const user = req.session.user;
        const result = await pool.query('SELECT * FROM records WHERE user_id = $1 ORDER BY id DESC', [user.id]);
        res.render('dashboard', { user: user, records: result.rows });
    } catch (e) {
        console.error("Dashboard error:", e);
        res.send("Error fetching dashboard data.");
    }
});

app.listen(port, () => {
    console.log(`Lekha website listening on port ${port}`);
});
