#ifndef DIGIT_H_
#define DIGIT_H_
// NOTE: THIS IS OUT OF DATE - KINDA. The order remains the same but there are now more pixels. 8 per 'segment'
// The order of pixels:
// D7               D6
//
//17	18	19	00    17	18	19	00
//16    			01    16    			01
//15	    		02    15	    		02
//14	21  20	03    14	21  20	03
//13	    		04    13	    		04
//12	    		05    12	    		05
//11          06    11          06
//10  09  08  07    10  09  08  07


/*
 * Each bit will be read, from the least-significant bit (the right)
 * If the value is one that indicates the pixel should be on
 * Note: you do not have to left pad 0s, but I will here
 */
enum DigitValue {
  // Numbers
  ZERO  = 0b00000000111111111111111111111111111111111111111111111111,
  ONE   = 0b00000000000000000000000000000000000000001111111111111111,
  TWO   = 0b11111111111111110000000011111111111111110000000011111111,
  THREE = 0b11111111111111110000000000000000111111111111111111111111,
  FOUR  = 0b11111111000000001111111100000000000000001111111111111111,
  FIVE  = 0b11111111111111111111111100000000111111111111111100000000,
  SIX   = 0b11111111111111111111111111111111111111111111111100000000,
  SEVEN = 0b00000000111111110000000000000000000000001111111111111111,
  EIGHT = 0b11111111111111111111111111111111111111111111111111111111,
  NINE  = 0b11111111111111111111111100000000111111111111111111111111,
  DASH  = 0b11111111000000000000000000000000000000000000000000000000,
  EMPTY = 0b00000000000000000000000000000000000000000000000000000000,

  // Alphabet - based on DSeg Font
  A     = 0b11111111111111111111111111111111000000001111111111111111,
  B     = 0b11111111000000011111111111111111111111111111111100000000,
  C     = 0b11111111000000000000000011111111111111110000000000000000,
  D     = 0b11111111000000000000000011111111111111111111111111111111,
  E     = 0b11111111111111111111111111111111111111110000000000000000,
  F     = 0b11111111111111111111111111111111000000000000000000000000,
  G     = 0b00000000111111111111111111111111111111111111111100000000,
  H     = 0b11111111000000001111111111111111000000001111111111111111,
  I     = 0b00000000000000000000000000000000000000001111111100000000,
  J     = 0b00000000000000000000000011111111111111111111111111111111,
  K     = 0b11111111111111111111111111111111000000001111111100000000,
  L     = 0b00000000000000001111111111111111111111110000000000000000,
  M     = 0b00000000111111111111111111111111000000001111111111111111,
  N     = 0b11111111000000000000000011111111000000001111111100000000,
  O     = 0b11111111000000000000000011111111111111111111111100000000,
  P     = 0b11111111111111111111111111111111000000000000000011111111,
  Q     = 0b11111111111111111111111100000000000000001111111111111111,
  R     = 0b11111111000000000000000011111111000000000000000000000000,
  S     = 0b11111111000000001111111100000000111111111111111100000000,
  T     = 0b11111111000000001111111111111111111111110000000000000000,
  U     = 0b00000000000000000000000011111111111111111111111100000000,
  V     = 0b00000000000000001111111111111111111111111111111111111111,
  W     = 0b11111111000000001111111111111111111111111111111111111111,
  X     = 0b11111111000000001111111111111111000000001111111111111111,
  Y     = 0b11111111000000001111111100000000111111111111111111111111,
  Z     = 0b000000001111111100000000011111111111111110000000011111111,
};

class Digit {
  DigitValue value;

 public:
  explicit Digit(int number) {
    switch (number) {
      case 0: value = ZERO; break;
      case 1: value = ONE; break;
      case 2: value = TWO; break;
      case 3: value = THREE; break;
      case 4: value = FOUR; break;
      case 5: value = FIVE; break;
      case 6: value = SIX; break;
      case 7: value = SEVEN; break;
      case 8: value = EIGHT; break;
      case 9: value = NINE; break;
      default: value = E; break;
    }
  }

  explicit Digit(DigitValue value) {
    this->value = value;
  }

  // Mostly for testing
  explicit Digit(char character) {
    switch (character) {
      case 'a': case 'A': value = A; break;
      case 'b': case 'B': value = B; break;
      case 'c': case 'C': value = C; break;
      case 'd': case 'D': value = D; break;
      case 'e': case 'E': value = E; break;
      case 'f': case 'F': value = F; break;
      case 'g': case 'G': value = G; break;
      case 'h': case 'H': value = H; break;
      case 'i': case 'I': value = I; break;
      case 'j': case 'J': value = J; break;
      case 'k': case 'K': value = K; break;
      case 'l': case 'L': value = L; break;
      case 'm': case 'M': value = M; break;
      case 'n': case 'N': value = N; break;
      case 'o': case 'O': value = O; break;
      case 'p': case 'P': value = P; break;
      case 'q': case 'Q': value = Q; break;
      case 'r': case 'R': value = R; break;
      case 's': case 'S': value = S; break;
      case 't': case 'T': value = T; break;
      case 'u': case 'U': value = U; break;
      case 'v': case 'V': value = V; break;
      case 'w': case 'W': value = W; break;
      case 'x': case 'X': value = X; break;
      case 'y': case 'Y': value = Y; break;
      case 'z': case 'Z': value = Z; break;
      default: value = E; break;
    }
  }

  DigitValue getDigitValue() {
    return value;
  }
};

#endif  // DIGIT_H_
