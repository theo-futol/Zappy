FROM node:22-alpine

WORKDIR /app

COPY bridge/package.json ./
RUN npm install

COPY bridge/ ./bridge/
COPY browser/ ./browser/

ENV ZAPPY_SERVER_HOST=server
ENV ZAPPY_SERVER_PORT=4242
ENV GUI_PORT=8081
ENV TILE_UPDATE_INTERVAL_MS=5000

EXPOSE 8081

CMD ["node", "bridge/bridge.js"]
