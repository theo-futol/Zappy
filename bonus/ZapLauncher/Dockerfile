FROM python:trixie

COPY requirements.txt .

RUN pip install -r requirements.txt

COPY . .

VOLUME /prompt_database

EXPOSE 8000

# Interactive API docs (Swagger UI / ReDoc) are served by FastAPI itself
# at /docs and /redoc once the container is running.
CMD ["uvicorn", "api:app", "--host", "0.0.0.0", "--port", "8000"]
