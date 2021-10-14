clownlzss is a lightweight, minimalist, graph-based LZSS framework.
Also included are a collection of compressors which utilise the framework.

Formats supported by the supplied utilities include:

* Kosinski - a compression format common to first-party Sega Mega Drive games

* Saxman - a format used by Sonic the Hedgehog 2, to compress its sound engine
  and music data (is actually a lightly-modified version of Haruhiko Okumura's
  1989 LZSS format)

* Chameleon - a format that saw use in Kid Chameleon and Sonic the Hedgehog 2's
  "Nick Arcade" prototype

* Rocket - a format used by Rocket Knight Adventures

* Rage - a format used by Streets of Rage 2 (is actually RLE, not LZSS)

* Kosinski+ - a modified version of Kosinski developed by the Sonic ROM-hacking
  community, which optimises the format for the Mega Drive's CPU, improving
  decompression speed at no cost to compression ratio

* Faxman - a modified version of Saxman designed to produce smaller files when
  used to compress SMPS music data

* Comper - another community-developed format, which is designed from the
  ground up with a focus on decompression speed on the Motorola 68000. This
  comes at a significant cost to compression ratio

clownlzss utilises graph theory to perform optimal compression: naive "greedy
algorithm" compressors prefer to compress the longest runs possible, but this
does not guarantee the best compression ratio. Graph-based compressors resolve
this by creating an "LZSS graph" - a weighted directed acyclic graph where each
node is a value in the uncompressed file, and each edge is an LZSS match. By
using a shortest-path algorithm, this graph can be used to compute the ideal
combination of matches needed to produce the smallest file.

This project is under the zlib licence.
