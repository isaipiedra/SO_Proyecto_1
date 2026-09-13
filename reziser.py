from PIL import Image
fileName = "og_up"
image = Image.open("images/" + fileName + ".png")

max_size = (10, 10)

image = image.convert("RGBA")
image.putalpha(128)

image.thumbnail(max_size, Image.Resampling.LANCZOS)

# Save the output
image.save("images/" + fileName[3:] + ".png")
print(f"New dimensions: {image.size}")  # e.g., (400, 266)
