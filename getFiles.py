import requests
from bs4 import BeautifulSoup
import os

url = "https://www.gutenberg.org/browse/scores/top-en.html"

html = requests.get(url).text
soup = BeautifulSoup(html, "html.parser")

os.makedirs("gutenberg100", exist_ok=True)

books = []

for link in soup.select("li a"):
    href = link.get("href", "")

    if href.startswith("/ebooks/") and href[8:].isdigit():
        book_id = href.split("/")[-1]
        books.append(book_id)

books = list(dict.fromkeys(books))[:100]

for i, book_id in enumerate(books, 1):
    url = f"https://www.gutenberg.org/cache/epub/{book_id}/pg{book_id}.txt"

    response = requests.get(url)

    if response.status_code == 200:
        with open(f"gutenberg100/{book_id}.txt", "wb") as f:
            f.write(response.content)

        print(f"[{i}/100] Downloaded {book_id}")
    else:
        print(f"[{i}/100] Failed: {book_id}")