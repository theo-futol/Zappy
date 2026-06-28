import dotenv from 'dotenv';

dotenv.config();

export const config = {
    tcp_port: parseInt(process.env.TCP_PORT, 10) || 4242;
    tcp_host: 'localhost';
    web_port: parseInt(process.env.WEB_PORT) || 8080; 
    host: process.env.HOST || 'localhost';
}
