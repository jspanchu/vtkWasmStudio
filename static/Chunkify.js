// Break a blob into individual blobs of `chunkSize` number of bytes.
export function chunkify(blob, chunkSize) {
  let numChunks = Math.ceil(blob.size / chunkSize);
  let i = 0;
  let chunks = [];
  while (i < numChunks) {
    let offset = (i++) * chunkSize;
    chunks.push(blob.slice(offset, offset + chunkSize));
  }
  return chunks;
}
