 #include <iostream>
 #include <string>
 #include <vector>
 #include <functional>
 #include <map>
 #include <sstream>
 #include <iomanip>
 #include <algorithm>
 #include <cmath>
 #include <cstring>
 #include <climits>
 #include <cassert>
 #include <bitset>
 #include <numeric>
 #include <cstdint>
  #include <termios.h>
 #include <unistd.h>
 #include <sys/ioctl.h>
 #include <signal.h>
 
 
 
 
 namespace Term {
 
 static struct termios orig_termios;
 static int COLS = 80, ROWS = 24;
 
 void getSize() {
     struct winsize w;
     ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
     COLS = w.ws_col > 0 ? w.ws_col : 80;
     ROWS = w.ws_row > 0 ? w.ws_row : 24;
 }
 
 void rawOn() {
     tcgetattr(STDIN_FILENO, &orig_termios);
     struct termios raw = orig_termios;
     raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);
     raw.c_iflag &= ~(IXON | ICRNL | BRKINT | INPCK | ISTRIP);
     raw.c_cflag |=  (CS8);
     raw.c_oflag &= ~(OPOST);
     raw.c_cc[VMIN]  = 0;
     raw.c_cc[VTIME] = 1;
     tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
 }
 
 void rawOff() {
     tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
 }
 
 void write_str(const std::string &s) { ::write(STDOUT_FILENO, s.c_str(), s.size()); }
 
 
 std::string esc(const std::string &s) { return "\x1b[" + s; }
 std::string mv(int r, int c)          { return esc(std::to_string(r)+";"+std::to_string(c)+"H"); }
 std::string clrline()                 { return esc("2K"); }
 std::string clrscr()                  { return esc("2J") + mv(1,1); }
 std::string hideCur()                 { return esc("?25l"); }
 std::string showCur()                 { return esc("?25h"); }
 
 
 std::string fg(int n)   { return esc("38;5;"+std::to_string(n)+"m"); }
 std::string bg(int n)   { return esc("48;5;"+std::to_string(n)+"m"); }
 std::string bold()      { return esc("1m"); }
 std::string dim()       { return esc("2m"); }
 std::string underline() { return esc("4m"); }
 std::string reverse()   { return esc("7m"); }
 std::string reset()     { return esc("0m"); }
 
 
 static const int C_GREEN      = 46;   
 static const int C_GREEN_DIM  = 28;
 static const int C_BORDER     = 34;
 static const int C_AMBER      = 214;
 static const int C_CYAN       = 51;
 static const int C_MAGENTA    = 201;
 static const int C_RED        = 196;
 static const int C_WHITE      = 231;
 static const int C_BG         = 232;  
 static const int C_PANEL_BG   = 233;
 static const int C_SEL_BG     = 22;
 static const int C_SEL_FG     = 46;
 static const int C_LABEL      = 178;
 static const int C_OUTPUT     = 51;
 
 void mouseOn()  { write_str("\x1b[?1000h\x1b[?1006h"); } 
 void mouseOff() { write_str("\x1b[?1006l\x1b[?1000l"); }
 
 
 struct Key {
     enum { NONE=0, CHAR, UP, DOWN, LEFT, RIGHT, ENTER, TAB,
            CTRL_A, CTRL_C, CTRL_L, CTRL_X, BACKSPACE, DEL, PGUP, PGDN,
            HOME, END, ESCAPE, F1,
            MOUSE_PRESS, MOUSE_RELEASE, MOUSE_SCROLL_UP, MOUSE_SCROLL_DOWN };
     int type;
     char ch;
     int mx, my;   
     int mbtn;     
 };
 
 Key readKey() {
     char c = 0;
     int n = ::read(STDIN_FILENO, &c, 1);
     if (n <= 0) return {Key::NONE, 0, 0, 0, 0};
 
     if (c == '\x1b') {
         char seq[32] = {};
         int nr = ::read(STDIN_FILENO, seq, 1);
         if (nr <= 0) return {Key::ESCAPE, 0, 0, 0, 0};
 
         if (seq[0] != '[' && seq[0] != 'O') return {Key::ESCAPE, 0, 0, 0, 0};
         ::read(STDIN_FILENO, seq+1, 1);
 
         
         if (seq[0] == '[' && seq[1] == '<') {
             char buf[32] = {};
             int bi = 0;
             char last = 0;
             while (bi < 30) {
                 char cc = 0;
                 if (::read(STDIN_FILENO, &cc, 1) <= 0) break;
                 if (cc == 'M' || cc == 'm') { last = cc; break; }
                 buf[bi++] = cc;
             }
             
             int btn=0, mx=0, my=0;
             sscanf(buf, "%d;%d;%d", &btn, &mx, &my);
             if (last == 'm') return {Key::MOUSE_RELEASE, 0, mx, my, btn};
             
             if (btn == 64) return {Key::MOUSE_SCROLL_UP,   0, mx, my, btn};
             if (btn == 65) return {Key::MOUSE_SCROLL_DOWN, 0, mx, my, btn};
             return {Key::MOUSE_PRESS, 0, mx, my, btn};
         }
 
         if (seq[0] == '[') {
             if (seq[1] >= '0' && seq[1] <= '9') {
                 char seq2 = 0;
                 ::read(STDIN_FILENO, &seq2, 1);
                 if (seq2 == '~') {
                     if (seq[1]=='5') return {Key::PGUP,0,0,0,0};
                     if (seq[1]=='6') return {Key::PGDN,0,0,0,0};
                     if (seq[1]=='1') return {Key::HOME,0,0,0,0};
                     if (seq[1]=='4') return {Key::END,0,0,0,0};
                     if (seq[1]=='3') return {Key::DEL,0,0,0,0};
                 }
             }
             switch(seq[1]) {
                 case 'A': return {Key::UP,0,0,0,0};
                 case 'B': return {Key::DOWN,0,0,0,0};
                 case 'C': return {Key::RIGHT,0,0,0,0};
                 case 'D': return {Key::LEFT,0,0,0,0};
                 case 'H': return {Key::HOME,0,0,0,0};
                 case 'F': return {Key::END,0,0,0,0};
             }
         }
         return {Key::ESCAPE,0,0,0,0};
     }
     if (c == '\r' || c == '\n') return {Key::ENTER,0,0,0,0};
     if (c == '\t')              return {Key::TAB,0,0,0,0};
     if (c == 127 || c == 8)    return {Key::BACKSPACE,0,0,0,0};
     if (c == 1)                 return {Key::CTRL_A,0,0,0,0};
     if (c == 3)                 return {Key::CTRL_C,0,0,0,0};
     if (c == 12)                return {Key::CTRL_L,0,0,0,0};
     if (c == 24)                return {Key::CTRL_X,0,0,0,0};
     if (c == 'q' || c == 'Q')  return {Key::CHAR,'q',0,0,0};
     return {Key::CHAR, c, 0, 0, 0};
 }
 
 } 
 
 
 
 
 namespace Enc {
 
 using Fn = std::function<std::string(const std::string&)>;
 
 
 
 static std::string toHexStr(const std::string &s, const std::string &sep=" ", bool upper=true) {
     std::ostringstream o;
     for (size_t i = 0; i < s.size(); i++) {
         if (i) o << sep;
         if (upper) o << std::uppercase;
         o << std::hex << std::setw(2) << std::setfill('0') << (unsigned)(unsigned char)s[i];
     }
     return o.str();
 }
 
 static std::string toBinStr(const std::string &s) {
     std::string r;
     for (size_t i = 0; i < s.size(); i++) {
         if (i) r += ' ';
         unsigned char c = s[i];
         for (int b = 7; b >= 0; b--) r += ((c >> b) & 1) ? '1' : '0';
     }
     return r;
 }
 
 static const std::string B64_CHARS =
     "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
 
 static std::string base64Enc(const std::string &s, bool urlSafe=false, bool padding=true) {
     std::string r;
     int val=0, valb=-6;
     for (unsigned char c : s) {
         val = (val<<8)+c; valb+=8;
         while (valb>=0) {
             r += B64_CHARS[(val>>valb)&0x3F];
             valb-=6;
         }
     }
     if (valb>-6) r += B64_CHARS[((val<<8)>>(valb+8))&0x3F];
     if (padding) while (r.size()%4) r += '=';
     if (urlSafe) { for(auto&c:r){if(c=='+')c='-';if(c=='/')c='_';} }
     return r;
 }
 
 static std::string base32Enc(const std::string &s) {
     const std::string a = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
     std::string r;
     int buf=0, bits=0;
     for (unsigned char c : s) {
         buf = (buf<<8)|c; bits+=8;
         while (bits>=5) { bits-=5; r+=a[(buf>>bits)&31]; }
     }
     if (bits>0) { r+=a[(buf<<(5-bits))&31]; }
     while(r.size()%8) r+='=';
     return r;
 }
 
 
 static std::string rotN(const std::string &s, int n) {
     std::string r;
     for (char c : s) {
         if (c>='a'&&c<='z') r+=(char)('a'+(c-'a'+n)%26);
         else if (c>='A'&&c<='Z') r+=(char)('A'+(c-'A'+n)%26);
         else r+=c;
     }
     return r;
 }
 
 
 static std::string vigenere(const std::string &text, const std::string &key, bool decode=false) {
     if (key.empty()) return "(key required – using 'KEY')";
     std::string k = key.empty() ? "KEY" : key;
     std::string r; int ki=0;
     for (char c : text) {
         if (isalpha(c)) {
             char base = isupper(c) ? 'A' : 'a';
             char kb = toupper(k[ki%k.size()])-'A';
             if (decode) r+=(char)(base+(c-base-kb+26)%26);
             else        r+=(char)(base+(c-base+kb)%26);
             ki++;
         } else r+=c;
     }
     return r;
 }
 
 
 static std::string xorKey(const std::string &s, uint8_t key) {
     std::string r;
     for (unsigned char c : s) r += (char)(c ^ key);
     return r;
 }
 
 
 static std::string urlEncode(const std::string &s) {
     std::ostringstream o;
     for (unsigned char c : s) {
         if (isalnum(c)||c=='-'||c=='_'||c=='.'||c=='~') o<<c;
         else o<<'%'<<std::uppercase<<std::hex<<std::setw(2)<<std::setfill('0')<<(int)c;
     }
     return o.str();
 }
 
 
 static std::string htmlEncode(const std::string &s) {
     std::string r;
     for (char c : s) {
         switch(c) {
             case '&': r+="&amp;"; break;
             case '<': r+="&lt;"; break;
             case '>': r+="&gt;"; break;
             case '"': r+="&quot;"; break;
             case '\'': r+="&#39;"; break;
             default: r+=c;
         }
     }
     return r;
 }
 
 
 static std::string unicodeCodepoints(const std::string &s) {
     std::string r;
     for (unsigned char c : s) {
         if (!r.empty()) r+=' ';
         std::ostringstream o;
         o << "U+" << std::uppercase << std::hex << std::setw(4) << std::setfill('0') << (int)c;
         r+=o.str();
     }
     return r;
 }
 
 
 static std::string morseEncode(const std::string &s) {
     static std::map<char,std::string> m = {
         {'A',".-"},{'B',"-..."},{'C',"-.-."},{'D',"-.."},{'E',"."},
         {'F',"..-."},{'G',"--."},{'H',"...."},{'I',".."},{'J',".---"},
         {'K',"-.-"},{'L',".-.."},{'M',"--"},{'N',"-."},{'O',"---"},
         {'P',".--."},{'Q',"--.-"},{'R',".-."},{'S',"..."},{'T',"-"},
         {'U',"..-"},{'V',"...-"},{'W',".--"},{'X',"-..-"},{'Y',"-.--"},
         {'Z',"--.."},{'0',"-----"},{'1',".----"},{'2',"..---"},
         {'3',"...--"},{'4',"....-"},{'5',"....."},{'6',"-....."},
         {'7',"--...."},{'8',"---..."},{'9',"----."},
         {'.',".-.-.-"},{',',"--..--"},{'?',"..--.."},{'!',"-.-.--"},
         {'/',"-..-."},{'-',"-....-"},{' '," / "}
     };
     std::string r;
     for (char c : s) {
         char u = toupper(c);
         if (m.count(u)) { if(!r.empty()&&r.back()!=' ')r+=' '; r+=m[u]; }
     }
     return r;
 }
 
 
 static std::string brailleEncode(const std::string &s) {
     
     static std::map<char,uint8_t> b = {
         {'a',1},{'b',3},{'c',9},{'d',25},{'e',17},{'f',11},{'g',27},{'h',19},
         {'i',10},{'j',26},{'k',5},{'l',7},{'m',13},{'n',29},{'o',21},{'p',15},
         {'q',31},{'r',23},{'s',14},{'t',30},{'u',37},{'v',39},{'w',58},{'x',45},
         {'y',61},{'z',53},{' ',0}
     };
     std::string r;
     for (char c : s) {
         char lo = tolower(c);
         uint8_t dots = b.count(lo) ? b[lo] : 0;
         
         uint32_t cp = 0x2800 + dots;
         
         r += (char)(0xE0 | (cp >> 12));
         r += (char)(0x80 | ((cp >> 6) & 0x3F));
         r += (char)(0x80 | (cp & 0x3F));
     }
     return r;
 }
 
 
 static std::string natoPhonetic(const std::string &s) {
     static std::map<char,std::string> m = {
         {'A',"Alpha"},{'B',"Bravo"},{'C',"Charlie"},{'D',"Delta"},{'E',"Echo"},
         {'F',"Foxtrot"},{'G',"Golf"},{'H',"Hotel"},{'I',"India"},{'J',"Juliet"},
         {'K',"Kilo"},{'L',"Lima"},{'M',"Mike"},{'N',"November"},{'O',"Oscar"},
         {'P',"Papa"},{'Q',"Quebec"},{'R',"Romeo"},{'S',"Sierra"},{'T',"Tango"},
         {'U',"Uniform"},{'V',"Victor"},{'W',"Whiskey"},{'X',"X-ray"},
         {'Y',"Yankee"},{'Z',"Zulu"},{'0',"Zero"},{'1',"One"},{'2',"Two"},
         {'3',"Three"},{'4',"Four"},{'5',"Five"},{'6',"Six"},{'7',"Seven"},
         {'8',"Eight"},{'9',"Nine"},{' ',"[SPACE]"}
     };
     std::string r;
     for (char c : s) {
         char u = toupper(c);
         if (m.count(u)) { if(!r.empty()) r+='-'; r+=m[u]; }
         else { if(!r.empty()) r+='-'; r+=c; }
     }
     return r;
 }
 
 
 static std::string leet(const std::string &s) {
     static std::map<char,std::string> m = {
         {'a',"4"},{'e',"3"},{'i',"1"},{'o',"0"},{'t',"7"},
         {'s',"5"},{'l',"1"},{'g',"9"},{'b',"8"},{'z',"2"}
     };
     std::string r;
     for (char c : s) {
         char lo = tolower(c);
         if (m.count(lo)) r+=m[lo]; else r+=c;
     }
     return r;
 }
 
 
 static std::string pigLatin(const std::string &s) {
     auto isVowel=[](char c){c=tolower(c);return c=='a'||c=='e'||c=='i'||c=='o'||c=='u';};
     std::istringstream iss(s); std::string word, r;
     while (iss>>word) {
         if (!r.empty()) r+=' ';
         if (isVowel(word[0])) { r+=word+"way"; continue; }
         size_t i=1;
         while(i<word.size()&&!isVowel(word[i])) i++;
         r+=word.substr(i)+word.substr(0,i)+"ay";
     }
     return r;
 }
 
 
 static std::string reverseStr(const std::string &s) {
     return std::string(s.rbegin(), s.rend());
 }
 
 
 static std::string reverseWords(const std::string &s) {
     std::istringstream iss(s);
     std::vector<std::string> words;
     std::string w;
     while(iss>>w) words.push_back(w);
     std::reverse(words.begin(),words.end());
     std::string r;
     for(auto&x:words){if(!r.empty())r+=' ';r+=x;}
     return r;
 }
 
 
 static std::string rot47(const std::string &s) {
     std::string r;
     for (char c : s) {
         if (c>=33&&c<=126) r+=(char)(33+(c-33+47)%94);
         else r+=c;
     }
     return r;
 }
 
 
 static std::string atbash(const std::string &s) {
     std::string r;
     for (char c : s) {
         if (c>='a'&&c<='z') r+=(char)('z'-(c-'a'));
         else if (c>='A'&&c<='Z') r+=(char)('Z'-(c-'A'));
         else r+=c;
     }
     return r;
 }
 
 
 static std::string upsideDown(const std::string &s) {
     static std::map<char,std::string> flip = {
         {'a',"ɐ"},{'b',"q"},{'c',"ɔ"},{'d',"p"},{'e',"ǝ"},{'f',"ɟ"},
         {'g',"ƃ"},{'h',"ɥ"},{'i',"ᴉ"},{'j',"ɾ"},{'k',"ʞ"},{'l',"l"},
         {'m',"ɯ"},{'n',"u"},{'o',"o"},{'p',"d"},{'q',"b"},{'r',"ɹ"},
         {'s',"s"},{'t',"ʇ"},{'u',"n"},{'v',"ʌ"},{'w',"ʍ"},{'x',"x"},
         {'y',"ʎ"},{'z',"z"},{'A',"∀"},{'B',"q"},{'C',"Ɔ"},{'D',"p"},
         {'E',"Ǝ"},{'F',"Ⅎ"},{'G',"פ"},{'H',"H"},{'I',"I"},{'J',"ɾ"},
         {'K',"ʞ"},{'L',"˥"},{'M',"W"},{'N',"N"},{'O',"O"},{'P',"Ԁ"},
         {'Q',"Q"},{'R',"ɹ"},{'S',"S"},{'T',"┴"},{'U',"∩"},{'V',"Λ"},
         {'W',"M"},{'X',"X"},{'Y',"⅄"},{'Z',"Z"},
         {'0',"0"},{'1',"Ɩ"},{'2',"ᄅ"},{'3',"Ɛ"},{'4',"ㄣ"},{'5',"ϛ"},
         {'6',"9"},{'7',"L"},{'8',"8"},{'9',"6"},
         {'.',"˙"},{',',"'"},{'?',"¿"},{'!',"¡"},{'(',")"},{')',"("},
         {'[',"["},{'{',"}"},{' '," "}
     };
     std::string r;
     for (auto it=s.rbegin();it!=s.rend();++it) {
         if (flip.count(*it)) r+=flip[*it]; else r+=*it;
     }
     return r;
 }
 
 
 static std::string zalgo(const std::string &s) {
     const char* up[] = {"\xcc\x81","\xcc\x82","\xcc\x83","\xcc\x84","\xcc\x85","\xcc\x87","\xcc\x88"};
     const char* dn[] = {"\xcd\x97","\xcd\x98","\xcd\x99","\xcd\xa2","\xcd\xa3"};
     std::string r;
     int idx=0;
     for (char c : s) {
         r+=c;
         if (c!=' ') {
             r+=up[idx%7]; r+=up[(idx+2)%7];
             r+=dn[idx%5];
         }
         idx++;
     }
     return r;
 }
 
 
 static std::string fullwidth(const std::string &s) {
     std::string r;
     for (unsigned char c : s) {
         if (c==' ') { r+=(char)0xE3;r+=(char)0x80;r+=(char)0x80; }
         else if (c>=0x21&&c<=0x7E) {
             uint32_t cp = 0xFF01+(c-0x21);
             r+=(char)(0xE0|(cp>>12));
             r+=(char)(0x80|((cp>>6)&0x3F));
             r+=(char)(0x80|(cp&0x3F));
         } else r+=c;
     }
     return r;
 }
 
 
 static std::string smallCaps(const std::string &s) {
     static std::map<char,std::string> m = {
         {'a',"ᴀ"},{'b',"ʙ"},{'c',"ᴄ"},{'d',"ᴅ"},{'e',"ᴇ"},{'f',"ꜰ"},
         {'g',"ɢ"},{'h',"ʜ"},{'i',"ɪ"},{'j',"ᴊ"},{'k',"ᴋ"},{'l',"ʟ"},
         {'m',"ᴍ"},{'n',"ɴ"},{'o',"ᴏ"},{'p',"ᴘ"},{'q',"Q"},{'r',"ʀ"},
         {'s',"ꜱ"},{'t',"ᴛ"},{'u',"ᴜ"},{'v',"ᴠ"},{'w',"ᴡ"},{'x',"x"},
         {'y',"ʏ"},{'z',"ᴢ"}
     };
     std::string r;
     for (char c : s) {
         char lo=tolower(c);
         if (m.count(lo)) r+=m[lo]; else r+=c;
     }
     return r;
 }
 
 
 static std::string superscript(const std::string &s) {
     static std::map<char,std::string> m = {
         {'0',"⁰"},{'1',"¹"},{'2',"²"},{'3',"³"},{'4',"⁴"},{'5',"⁵"},
         {'6',"⁶"},{'7',"⁷"},{'8',"⁸"},{'9',"⁹"},{'+', "⁺"},{'-',"⁻"},
         {'=',"⁼"},{'(',"⁽"},{')',"⁾"},{'a',"ᵃ"},{'b',"ᵇ"},{'c',"ᶜ"},
         {'d',"ᵈ"},{'e',"ᵉ"},{'f',"ᶠ"},{'g',"ᵍ"},{'h',"ʰ"},{'i',"ⁱ"},
         {'j',"ʲ"},{'k',"ᵏ"},{'l',"ˡ"},{'m',"ᵐ"},{'n',"ⁿ"},{'o',"ᵒ"},
         {'p',"ᵖ"},{'r',"ʳ"},{'s',"ˢ"},{'t',"ᵗ"},{'u',"ᵘ"},{'v',"ᵛ"},
         {'w',"ʷ"},{'x',"ˣ"},{'y',"ʸ"},{'z',"ᶻ"}
     };
     std::string r;
     for (char c : s) { if(m.count(c))r+=m[c]; else r+=c; }
     return r;
 }
 
 
 static std::string subscript(const std::string &s) {
     static std::map<char,std::string> m = {
         {'0',"₀"},{'1',"₁"},{'2',"₂"},{'3',"₃"},{'4',"₄"},{'5',"₅"},
         {'6',"₆"},{'7',"₇"},{'8',"₈"},{'9',"₉"},{'+', "₊"},{'-',"₋"},
         {'=',"₌"},{'(',"₍"},{')',"₎"},{'a',"ₐ"},{'e',"ₑ"},{'h',"ₕ"},
         {'i',"ᵢ"},{'j',"ⱼ"},{'k',"ₖ"},{'l',"ₗ"},{'m',"ₘ"},{'n',"ₙ"},
         {'o',"ₒ"},{'p',"ₚ"},{'r',"ᵣ"},{'s',"ₛ"},{'t',"ₜ"},{'u',"ᵤ"},
         {'v',"ᵥ"},{'x',"ₓ"}
     };
     std::string r;
     for (char c : s) { if(m.count(c))r+=m[c]; else r+=c; }
     return r;
 }
 
 
 static std::string bubbleText(const std::string &s) {
     std::string r;
     for (char c : s) {
         if (c>='A'&&c<='Z') {
             uint32_t cp = 0x24B6+(c-'A');
             r+=(char)(0xE2); r+=(char)(0x92+((cp-0x2400)>>6)); r+=(char)(0x80|(cp&0x3F));
         } else if (c>='a'&&c<='z') {
             uint32_t cp = 0x24D0+(c-'a');
             r+=(char)(0xE2); r+=(char)(0x93+((cp-0x2400)>>6)); r+=(char)(0x80|(cp&0x3F));
         } else if (c>='0'&&c<='9') {
             if (c=='0') { r+="\xe2\x93\xaa"; }
             else {
                 uint32_t cp = 0x2460+(c-'1');
                 r+=(char)(0xE2); r+=(char)(0x91+((cp-0x2400)>>6)); r+=(char)(0x80|(cp&0x3F));
             }
         } else r+=c;
     }
     return r;
 }
 
 
 static std::string mathBold(const std::string &s) {
     std::string r;
     for (char c : s) {
         uint32_t cp = 0;
         if (c>='A'&&c<='Z') cp=0x1D400+(c-'A');
         else if (c>='a'&&c<='z') cp=0x1D41A+(c-'a');
         else if (c>='0'&&c<='9') { uint32_t base=0x1D7CE; cp=base+(uint32_t)(c-'0'); }
         if (cp) {
             
             r+=(char)(0xF0|(cp>>18));
             r+=(char)(0x80|((cp>>12)&0x3F));
             r+=(char)(0x80|((cp>>6)&0x3F));
             r+=(char)(0x80|(cp&0x3F));
         } else r+=c;
     }
     return r;
 }
 
 
 static std::string mathItalic(const std::string &s) {
     std::string r;
     for (char c : s) {
         uint32_t cp = 0;
         if (c>='A'&&c<='Z') cp=0x1D434+(c-'A');
         else if (c>='a'&&c<='z') { uint32_t base=0x1D44E; cp=(c=='h')?(uint32_t)0x210E:base+(uint32_t)(c-'a'); }
         if (cp) {
             r+=(char)(0xF0|(cp>>18));
             r+=(char)(0x80|((cp>>12)&0x3F));
             r+=(char)(0x80|((cp>>6)&0x3F));
             r+=(char)(0x80|(cp&0x3F));
         } else r+=c;
     }
     return r;
 }
 
 
 static std::string mathDouble(const std::string &s) {
     std::string r;
     for (char c : s) {
         uint32_t cp = 0;
         if (c>='A'&&c<='Z') cp=0x1D538+(c-'A');
         else if (c>='a'&&c<='z') cp=0x1D552+(c-'a');
         else if (c>='0'&&c<='9') cp=0x1D7D8+(c-'0');
         if (cp) {
             r+=(char)(0xF0|(cp>>18));
             r+=(char)(0x80|((cp>>12)&0x3F));
             r+=(char)(0x80|((cp>>6)&0x3F));
             r+=(char)(0x80|(cp&0x3F));
         } else r+=c;
     }
     return r;
 }
 
 
 static std::string toOctal(const std::string &s) {
     std::string r;
     for (size_t i=0;i<s.size();i++) {
         if (i) r+=' ';
         std::ostringstream o;
         o<<std::oct<<std::setw(3)<<std::setfill('0')<<(unsigned)(unsigned char)s[i];
         r+=o.str();
     }
     return r;
 }
 
 
 static std::string toDecimal(const std::string &s) {
     std::string r;
     for (size_t i=0;i<s.size();i++) {
         if(i) r+=' ';
         r+=std::to_string((unsigned)(unsigned char)s[i]);
     }
     return r;
 }
 
 
 static std::string toHex0x(const std::string &s) {
     std::string r;
     for (size_t i=0;i<s.size();i++) {
         if(i) r+=' ';
         std::ostringstream o;
         o<<"0x"<<std::uppercase<<std::hex<<std::setw(2)<<std::setfill('0')<<(unsigned)(unsigned char)s[i];
         r+=o.str();
     }
     return r;
 }
 
 
 static std::string cStringLiteral(const std::string &s) {
     std::string r = "\"";
     for (unsigned char c : s) {
         switch(c) {
             case '\\': r+="\\\\"; break;
             case '"':  r+="\\\""; break;
             case '\n': r+="\\n";  break;
             case '\r': r+="\\r";  break;
             case '\t': r+="\\t";  break;
             default:
                 if (c<32||c>126) {
                     std::ostringstream o;
                     o<<"\\x"<<std::hex<<std::setw(2)<<std::setfill('0')<<(int)c;
                     r+=o.str();
                 } else r+=c;
         }
     }
     r+='"';
     return r;
 }
 
 
 static std::string pythonBytes(const std::string &s) {
     std::string r = "b\"";
     for (unsigned char c : s) {
         if (c=='"') r+="\\\"";
         else if (c=='\\') r+="\\\\";
         else if (c<32||c>126) {
             std::ostringstream o;
             o<<"\\x"<<std::hex<<std::setw(2)<<std::setfill('0')<<(int)c;
             r+=o.str();
         } else r+=c;
     }
     r+='"';
     return r;
 }
 
 
 static std::string jsonUnicode(const std::string &s) {
     std::string r;
     for (unsigned char c : s) {
         if (c<0x80) {
             std::ostringstream o;
             o<<"\\u"<<std::uppercase<<std::hex<<std::setw(4)<<std::setfill('0')<<(int)c;
             r+=o.str();
         } else r+=c;
     }
     return r;
 }
 
 
 static std::string hexDump(const std::string &s) {
     std::string r;
     size_t n=std::min(s.size(),(size_t)64); 
     for (size_t i=0;i<n;i+=16) {
         std::ostringstream line;
         line<<std::hex<<std::setw(8)<<std::setfill('0')<<i<<":  ";
         std::string ascii;
         for (size_t j=0;j<16;j++) {
             if (i+j<n) {
                 unsigned char c=s[i+j];
                 line<<std::uppercase<<std::hex<<std::setw(2)<<std::setfill('0')<<(int)c<<' ';
                 ascii+=(c>=32&&c<127)?(char)c:'.';
             } else { line<<"   "; ascii+=' '; }
             if (j==7) { line<<' '; }
         }
         line<<"  |"<<ascii<<"|";
         if (!r.empty()) r+='\n';
         r+=line.str();
     }
     if (s.size()>64) r+="\n... ("+std::to_string(s.size()-64)+" more bytes)";
     return r;
 }
 
 
 static std::string bacon(const std::string &s) {
     static std::map<char,std::string> m = {
         {'A',"AAAAA"},{'B',"AAAAB"},{'C',"AAABA"},{'D',"AAABB"},{'E',"AABAA"},
         {'F',"AABAB"},{'G',"AABBA"},{'H',"AABBB"},{'I',"ABAAA"},{'J',"ABAAA"},
         {'K',"ABAAB"},{'L',"ABABA"},{'M',"ABABB"},{'N',"ABBAA"},{'O',"ABBAB"},
         {'P',"ABBBA"},{'Q',"ABBBB"},{'R',"BAAAA"},{'S',"BAAAB"},{'T',"BAABA"},
         {'U',"BAABB"},{'V',"BAABB"},{'W',"BABAA"},{'X',"BABAB"},{'Y',"BABBA"},
         {'Z',"BABBB"}
     };
     std::string r;
     for (char c : s) {
         char u=toupper(c);
         if (m.count(u)) { if(!r.empty()&&r.back()!=' ')r+=' '; r+=m[u]; }
         else if (c==' ') r+="  ";
     }
     return r;
 }
 
 
 static std::string tapCode(const std::string &s) {
     
     static std::map<char,std::pair<int,int>> m = {
         {'A',{1,1}},{'B',{1,2}},{'C',{1,3}},{'D',{1,4}},{'E',{1,5}},
         {'F',{2,1}},{'G',{2,2}},{'H',{2,3}},{'I',{2,4}},{'J',{2,5}},
         {'K',{1,3}},{'L',{3,1}},{'M',{3,2}},{'N',{3,3}},{'O',{3,4}},
         {'P',{3,5}},{'Q',{4,1}},{'R',{4,2}},{'S',{4,3}},{'T',{4,4}},
         {'U',{4,5}},{'V',{5,1}},{'W',{5,2}},{'X',{5,3}},{'Y',{5,4}},
         {'Z',{5,5}}
     };
     std::string r;
     for (char c : s) {
         char u=toupper(c);
         if (m.count(u)) {
             if (!r.empty()) r+=' ';
             auto [row,col]=m[u];
             for(int i=0;i<row;i++){if(i)r+='.';r+='.';}r+=' ';
             for(int i=0;i<col;i++){if(i)r+='.';r+='.';}
         }
     }
     return r;
 }
 
 
 static std::string polybius(const std::string &s) {
     
     static const std::string sq="ABCDEFGHIKLMNOPQRSTUVWXYZ";
     std::string r;
     for (char c : s) {
         char u=toupper(c);
         if (u=='J') u='I';
         size_t pos=sq.find(u);
         if (pos!=std::string::npos) {
             if(!r.empty()) r+=' ';
             r+=std::to_string(pos/5+1);
             r+=std::to_string(pos%5+1);
         } else if (c==' ') r+="00";
     }
     return r;
 }
 
 
 static std::string affine(const std::string &s, int a=3, int b=7) {
     std::string r;
     for (char c : s) {
         if (isalpha(c)) {
             char base=isupper(c)?'A':'a';
             r+=(char)(base+((a*(c-base)+b)%26));
         } else r+=c;
     }
     return r;
 }
 
 
 static std::string railFence(const std::string &s, int rails=3) {
     if (rails<2||s.empty()) return s;
     std::vector<std::string> fence(rails);
     int rail=0, dir=1;
     for (char c : s) {
         fence[rail]+=c;
         if (rail==0) dir=1;
         else if (rail==rails-1) dir=-1;
         rail+=dir;
     }
     std::string r;
     for (auto &row:fence) r+=row;
     return r;
 }
 
 
 static std::string reverseEachWord(const std::string &s) {
     std::istringstream iss(s); std::string w,r;
     while(iss>>w){
         if(!r.empty())r+=' ';
         r+=std::string(w.rbegin(),w.rend());
     }
     return r;
 }
 
 
 static std::string alternatingCase(const std::string &s) {
     std::string r; bool up=true;
     for (char c : s) {
         if (isalpha(c)) { r+=up?toupper(c):tolower(c); up=!up; }
         else r+=c;
     }
     return r;
 }
 
 
 static std::string toCamelCase(const std::string &s) {
     std::istringstream iss(s); std::string w,r;
     bool first=true;
     while(iss>>w){
         if(first){for(auto&c:w)c=tolower(c);r+=w;first=false;}
         else {w[0]=toupper(w[0]);for(size_t i=1;i<w.size();i++)w[i]=tolower(w[i]);r+=w;}
     }
     return r;
 }
 
 
 static std::string toSnakeCase(const std::string &s) {
     std::string r;
     for(char c:s){ if(c==' ')r+='_'; else r+=tolower(c); }
     return r;
 }
 
 
 static std::string toKebabCase(const std::string &s) {
     std::string r;
     for(char c:s){ if(c==' ')r+='-'; else r+=tolower(c); }
     return r;
 }
 
 
 static std::string toScreamingSnake(const std::string &s) {
     std::string r;
     for(char c:s){ if(c==' ')r+='_'; else r+=toupper(c); }
     return r;
 }
 
 
 static std::string toUpper(const std::string &s) {
     std::string r=s; for(auto&c:r)c=toupper(c); return r;
 }
 static std::string toLower(const std::string &s) {
     std::string r=s; for(auto&c:r)c=tolower(c); return r;
 }
 static std::string toTitle(const std::string &s) {
     std::string r=s; bool next=true;
     for(auto&c:r){if(c==' ')next=true;else if(next){c=toupper(c);next=false;}}
     return r;
 }
 
 
 static std::string xorFF(const std::string &s) { return xorKey(s,0xFF); }
 static std::string xorAA(const std::string &s) { return xorKey(s,0xAA); }
 static std::string xorDeadBeef(const std::string &s) {
     static const uint8_t key[]={0xDE,0xAD,0xBE,0xEF};
     std::string r;
     for(size_t i=0;i<s.size();i++) r+=(char)((unsigned char)s[i]^key[i%4]);
     return r;
 }
 
 
 static std::string toIntBigEndian(const std::string &s) {
     std::string r;
     unsigned long long v=0;
     size_t n=std::min(s.size(),(size_t)8);
     for(size_t i=0;i<n;i++) v=(v<<8)|(unsigned char)s[i];
     return std::to_string(v)+" ("+std::to_string(n)+" bytes BE)";
 }
 
 
 static std::string rot1(const std::string &s){return rotN(s,1);}
 static std::string rot13(const std::string &s){return rotN(s,13);}
 static std::string rot18(const std::string &s){
     std::string r;
     for(char c:s){
         if(c>='0'&&c<='9') r+=(char)('0'+(c-'0'+5)%10);
         else if(isalpha(c)) r+=rotN(std::string(1,c),13)[0];
         else r+=c;
     }
     return r;
 }
 
 
 static std::string phoneticDigits(const std::string &s) {
     static std::map<char,std::string> m={
         {'0',"Zero"},{'1',"One"},{'2',"Two"},{'3',"Three"},{'4',"Four"},
         {'5',"Five"},{'6',"Six"},{'7',"Seven"},{'8',"Eight"},{'9',"Nine"},
         {'+',"Plus"},{'-',"Minus"},{'.',"Point"},{' ',"Space"}
     };
     std::string r;
     for(char c:s){ if(m.count(c)){if(!r.empty())r+='-';r+=m[c];}else r+=c; }
     return r;
 }
 
 
 static std::string soundex(const std::string &s) {
     static std::map<char,char> code={
         {'B','1'},{'F','1'},{'P','1'},{'V','1'},
         {'C','2'},{'G','2'},{'J','2'},{'K','2'},{'Q','2'},{'S','2'},{'X','2'},{'Z','2'},
         {'D','3'},{'T','3'},
         {'L','4'},
         {'M','5'},{'N','5'},
         {'R','6'}
     };
     std::istringstream iss(s); std::string word,r;
     while(iss>>word){
         std::string sd;
         char prev=0;
         for(char c:word){
             c=toupper(c);
             if(isalpha(c)){
                 if(sd.empty()){sd+=c;prev=code.count(c)?code[c]:0;}
                 else{
                     char cod=code.count(c)?code[c]:0;
                     if(cod&&cod!=prev){sd+=cod;}
                     if(cod) prev=cod; else prev=0;
                 }
             }
         }
         while(sd.size()<4)sd+='0';
         if(!r.empty())r+=' ';
         r+=sd.substr(0,4);
     }
     return r;
 }
 
 
 static std::string numericWords(const std::string &s) {
     std::string r;
     for(char c:s){
         if(!r.empty())r+='-';
         r+=std::to_string((int)(unsigned char)c);
     }
     return r;
 }
 
 
 static std::string binarySpaced(const std::string &s) { return toBinStr(s); }
 
 
 static std::string binaryStream(const std::string &s) {
     std::string r;
     for(unsigned char c:s)for(int b=7;b>=0;b--)r+=(char)('0'+((c>>b)&1));
     return r;
 }
 
 
 static std::string hexLower(const std::string &s){
     std::string r;
     for(size_t i=0;i<s.size();i++){
         if(i)r+=' ';
         std::ostringstream o;
         o<<std::hex<<std::setw(2)<<std::setfill('0')<<(unsigned)(unsigned char)s[i];
         r+=o.str();
     }
     return r;
 }
 
 
 static std::string hexCompact(const std::string &s){return toHexStr(s,"",true);}
 
 
 static std::string hexColon(const std::string &s){return toHexStr(s,":",true);}
 
 
 static std::string hexCArray(const std::string &s){
     std::string r="{ ";
     for(size_t i=0;i<s.size();i++){
         if(i)r+=", ";
         std::ostringstream o;
         o<<"0x"<<std::uppercase<<std::hex<<std::setw(2)<<std::setfill('0')<<(unsigned)(unsigned char)s[i];
         r+=o.str();
     }
     return r+" }";
 }
 
 
 static uint32_t djb2(const std::string &s){
     uint32_t h=5381;
     for(unsigned char c:s) h=((h<<5)+h)+c;
     return h;
 }
 static std::string djb2Hash(const std::string &s){
     std::ostringstream o;
     o<<"0x"<<std::uppercase<<std::hex<<djb2(s);
     return o.str();
 }
 
 static uint32_t fnv1a(const std::string &s){
     uint32_t h=2166136261u;
     for(unsigned char c:s){ h^=c; h*=16777619u; }
     return h;
 }
 static std::string fnv1aHash(const std::string &s){
     std::ostringstream o;
     o<<"0x"<<std::uppercase<<std::hex<<fnv1a(s);
     return o.str();
 }
 
 static uint32_t adler32(const std::string &s){
     uint32_t a=1,b=0;
     for(unsigned char c:s){ a=(a+c)%65521; b=(b+a)%65521; }
     return (b<<16)|a;
 }
 static std::string adler32Hash(const std::string &s){
     std::ostringstream o;
     o<<"0x"<<std::uppercase<<std::hex<<std::setw(8)<<std::setfill('0')<<adler32(s);
     return o.str();
 }
 
 
 static uint8_t crc8(const std::string &s){
     uint8_t crc=0xFF;
     for(unsigned char c:s){
         crc^=c;
         for(int i=0;i<8;i++) crc=(crc&0x80)?(crc<<1)^0x31:(crc<<1);
     }
     return crc;
 }
 static std::string crc8Hash(const std::string &s){
     std::ostringstream o;
     o<<"0x"<<std::uppercase<<std::hex<<std::setw(2)<<std::setfill('0')<<(int)crc8(s);
     return o.str();
 }
 
 
 static uint16_t crc16(const std::string &s){
     uint16_t crc=0xFFFF;
     for(unsigned char c:s){
         crc^=(uint16_t)c<<8;
         for(int i=0;i<8;i++) crc=(crc&0x8000)?(crc<<1)^0x1021:(crc<<1);
     }
     return crc;
 }
 static std::string crc16Hash(const std::string &s){
     std::ostringstream o;
     o<<"0x"<<std::uppercase<<std::hex<<std::setw(4)<<std::setfill('0')<<crc16(s);
     return o.str();
 }
 
 
 static std::string byteSum(const std::string &s){
     uint32_t sum=0; for(unsigned char c:s)sum+=c;
     std::ostringstream o; o<<"0x"<<std::uppercase<<std::hex<<sum<<" (dec "<<sum<<")";
     return o.str();
 }
 static std::string xorChecksum(const std::string &s){
     uint8_t x=0; for(unsigned char c:s)x^=c;
     std::ostringstream o; o<<"0x"<<std::uppercase<<std::hex<<std::setw(2)<<std::setfill('0')<<(int)x;
     return o.str();
 }
 
 
 static std::string charStats(const std::string &s){
     int letters=0,digits=0,spaces=0,punct=0,other=0;
     for(char c:s){
         if(isalpha(c))letters++;
         else if(isdigit(c))digits++;
         else if(isspace(c))spaces++;
         else if(ispunct(c))punct++;
         else other++;
     }
     return "letters="+std::to_string(letters)+" digits="+std::to_string(digits)+
            " spaces="+std::to_string(spaces)+" punct="+std::to_string(punct)+
            " other="+std::to_string(other)+" total="+std::to_string(s.size());
 }
 
 
 static std::string freqAnalysis(const std::string &s){
     std::map<char,int> freq;
     for(char c:s)freq[c]++;
     std::vector<std::pair<int,char>> v;
     for(auto&p:freq)v.push_back({p.second,p.first});
     std::sort(v.rbegin(),v.rend());
     std::string r;
     for(size_t i=0;i<std::min(v.size(),(size_t)10);i++){
         if(!r.empty())r+=' ';
         char c=v[i].second;
         std::string cs;
         if(c==' ')cs="SPC";
         else if(c=='\n')cs="NL";
         else cs=std::string(1,c);
         r+="'"+cs+"':"+std::to_string(v[i].first);
     }
     return r;
 }
 
 
 static std::string wordFreq(const std::string &s){
     std::map<std::string,int> freq;
     std::istringstream iss(s); std::string w;
     while(iss>>w){for(auto&c:w)c=tolower(c);freq[w]++;}
     std::vector<std::pair<int,std::string>> v;
     for(auto&p:freq)v.push_back({p.second,p.first});
     std::sort(v.rbegin(),v.rend());
     std::string r;
     for(size_t i=0;i<std::min(v.size(),(size_t)8);i++){
         if(!r.empty())r+=' ';
         r+='"'+v[i].second+"\":"+std::to_string(v[i].first);
     }
     return r;
 }
 
 
 static std::string entropy(const std::string &s){
     if(s.empty())return "0.000 bits/symbol";
     std::map<char,int> freq;
     for(char c:s)freq[c]++;
     double e=0,n=s.size();
     for(auto&p:freq){double p2=p.second/n;e-=p2*log2(p2);}
     std::ostringstream o;
     o<<std::fixed<<std::setprecision(4)<<e<<" bits/symbol (max="<<log2(freq.size())<<")";
     return o.str();
 }
 
 
 static std::string bitCount(const std::string &s){
     int ones=0,zeros=0;
     for(unsigned char c:s)for(int b=0;b<8;b++){if((c>>b)&1)ones++;else zeros++;}
     return "1-bits="+std::to_string(ones)+" 0-bits="+std::to_string(zeros)+
            " total="+std::to_string(ones+zeros);
 }
 
 
 static std::string sumCodepoints(const std::string &s){
     long long sum=0; for(unsigned char c:s)sum+=c;
     return std::to_string(sum);
 }
 static std::string productCodepoints(const std::string &s){
     
     long long prod=1; for(unsigned char c:s)if(c)prod=(prod*c)%1000000007LL;
     return std::to_string(prod)+" (mod 10^9+7)";
 }
 
 
 static std::string asciiBlock(const std::string &s) {
     
     std::string top="┌", mid="│", bot="└";
     for(char c:s){top+="──";mid+=std::string(1,toupper(c))+" ";bot+="──";}
     return top+"┐\n"+mid+"│\n"+bot+"┘";
 }
 
 
 static std::string baconAB(const std::string &s){return bacon(s);}
 
 
 static std::string unicodeEscape(const std::string &s){
     std::string r;
     for(unsigned char c:s){
         std::ostringstream o;
         o<<"\\u"<<std::uppercase<<std::hex<<std::setw(4)<<std::setfill('0')<<(int)c;
         r+=o.str();
     }
     return r;
 }
 
 
 static std::string percentEncodeAll(const std::string &s){
     std::string r;
     for(unsigned char c:s){
         std::ostringstream o;
         o<<'%'<<std::uppercase<<std::hex<<std::setw(2)<<std::setfill('0')<<(int)c;
         r+=o.str();
     }
     return r;
 }
 
 
 static std::string xmlEncode(const std::string &s){
     std::string r;
     for(char c:s){
         switch(c){
             case '&': r+="&amp;"; break;
             case '<': r+="&lt;"; break;
             case '>': r+="&gt;"; break;
             case '"': r+="&quot;"; break;
             case '\'': r+="&apos;"; break;
             default: if((unsigned char)c<32){
                 std::ostringstream o; o<<"&#x"<<std::hex<<(int)c<<";"; r+=o.str();
             } else r+=c;
         }
     }
     return r;
 }
 
 
 static std::string asciiCompat(const std::string &s){
     bool hasHigh=false;
     for(unsigned char c:s)if(c>127){hasHigh=true;break;}
     if(!hasHigh)return "xn--"+s+" (no non-ASCII chars)";
     std::string basic,r="xn--";
     for(char c:s)if((unsigned char)c<128)basic+=c;
     r+=basic;
     return r+"-(simplified)";
 }
 
 
 static std::string stringInfo(const std::string &s){
     size_t chars=s.size();
     size_t words=0; bool inw=false;
     for(char c:s){if(!isspace(c)){if(!inw){words++;inw=true;}}else inw=false;}
     size_t lines=1; for(char c:s)if(c=='\n')lines++;
     return "chars="+std::to_string(chars)+
            " words="+std::to_string(words)+
            " lines="+std::to_string(lines)+
            " bytes="+std::to_string(chars);
 }
 
 
 static std::string chunks(const std::string &s, int n=8){
     std::string r;
     for(size_t i=0;i<s.size();i++){
         if(i&&i%n==0)r+=' ';
         r+=s[i];
     }
     return r;
 }
 
 
 static std::string nullPadded(const std::string &s){
     std::string r;
     for(unsigned char c:s){
         std::ostringstream o;
         o<<std::uppercase<<std::hex<<std::setw(2)<<std::setfill('0')<<(int)c;
         r+=o.str()+"00 ";
     }
     return r;
 }
 
 
 static std::string utf16BE(const std::string &s){
     std::string r;
     for(unsigned char c:s){
         std::ostringstream o;
         o<<"00"<<std::uppercase<<std::hex<<std::setw(2)<<std::setfill('0')<<(int)c<<" ";
         r+=o.str();
     }
     return r;
 }
 
 
 static std::string utf32BE(const std::string &s){
     std::string r;
     for(unsigned char c:s){
         std::ostringstream o;
         o<<"000000"<<std::uppercase<<std::hex<<std::setw(2)<<std::setfill('0')<<(int)c<<" ";
         r+=o.str();
     }
     return r;
 }
 
 
 static std::string runLength(const std::string &s){
     if(s.empty())return "";
     std::string r;
     char cur=s[0]; int cnt=1;
     for(size_t i=1;i<s.size();i++){
         if(s[i]==cur)cnt++;
         else{r+=std::to_string(cnt)+cur;cur=s[i];cnt=1;}
     }
     r+=std::to_string(cnt)+cur;
     return r;
 }
 
 
 static std::string runLengthBin(const std::string &s){
     std::string bits;
     for(unsigned char c:s)for(int b=7;b>=0;b--)bits+=(char)('0'+((c>>b)&1));
     return runLength(bits);
 }
 
 
 static std::string quotedPrintable(const std::string &s){
     std::string r;
     for(unsigned char c:s){
         if((c>=33&&c<=126&&c!='=')) r+=c;
         else{
             std::ostringstream o;
             o<<'='<<std::uppercase<<std::hex<<std::setw(2)<<std::setfill('0')<<(int)c;
             r+=o.str();
         }
     }
     return r;
 }
 
 
 static std::string uuencode(const std::string &s){
     std::string r;
     size_t n=std::min(s.size(),(size_t)45);
     r+=(char)(n+32);
     size_t i=0;
     while(i<n){
         unsigned char b0=i<n?(unsigned char)s[i++]:0;
         unsigned char b1=i<n?(unsigned char)s[i++]:0;
         unsigned char b2=i<n?(unsigned char)s[i++]:0;
         r+=(char)((b0>>2)+32);
         r+=(char)(((b0<<4)|(b1>>4))&0x3F)+32;
         r+=(char)(((b1<<2)|(b2>>6))&0x3F)+32;
         r+=(char)((b2&0x3F)+32);
     }
     if(s.size()>45)r+="... (truncated)";
     return r;
 }
 
 
 static std::string toRoman(const std::string &s){
     int n=0;
     try{n=std::stoi(s);}catch(...){
         
         for(unsigned char c:s)n+=c;
     }
     if(n<=0||n>3999)return "(out of range: 1-3999, got "+std::to_string(n)+")";
     static std::vector<std::pair<int,std::string>> vals={
         {1000,"M"},{900,"CM"},{500,"D"},{400,"CD"},{100,"C"},{90,"XC"},
         {50,"L"},{40,"XL"},{10,"X"},{9,"IX"},{5,"V"},{4,"IV"},{1,"I"}
     };
     std::string r;
     for(auto&[v,sym]:vals)while(n>=v){r+=sym;n-=v;}
     return r;
 }
 
 
 static std::string tempConvert(const std::string &s){
     double v=0;
     try{v=std::stod(s);}catch(...){return "need numeric input";}
     std::ostringstream o;
     o<<std::fixed<<std::setprecision(2);
     o<<"C→F: "<<v*9/5+32<<"°F  |  "
      <<"C→K: "<<v+273.15<<"K  |  "
      <<"F→C: "<<(v-32)*5/9<<"°C  |  "
      <<"K→C: "<<v-273.15<<"°C";
     return o.str();
 }
 
 
 static std::string multiBase(const std::string &s){
     long long v=0;
     try{v=std::stoll(s);}catch(...){
         
         for(unsigned char c:s)v=(v<<8)|c;
     }
     std::ostringstream o;
     o<<"dec="<<v
      <<" hex=0x"<<std::uppercase<<std::hex<<v
      <<" oct=0"<<std::oct<<v
      <<" bin=";
     if(v==0) o<<"0";
     else { std::string b; long long t=v; while(t){b=(char)('0'+(t&1))+b;t>>=1;} o<<b; }
     return o.str();
 }
 
 
 
 
 static std::string fromCharcode(const std::string &s){
     std::string r; std::istringstream iss(s); std::string tok;
     while(iss>>tok){
         try{
             int base=10;
             if(tok.size()>2&&tok[0]=='0'&&(tok[1]=='x'||tok[1]=='X')){base=16;tok=tok.substr(2);}
             else if(tok.size()>1&&tok[0]=='0'){base=8;tok=tok.substr(1);}
             long cp=std::stol(tok,nullptr,base);
             if(cp>=0&&cp<128)r+=(char)cp;
             else if(cp<0x800){r+=(char)(0xC0|(cp>>6));r+=(char)(0x80|(cp&0x3F));}
             else if(cp<0x10000){r+=(char)(0xE0|(cp>>12));r+=(char)(0x80|((cp>>6)&0x3F));r+=(char)(0x80|(cp&0x3F));}
         }catch(...){}
     }
     return r.empty()?"(no valid charcodes found)":r;
 }
 
 
 static std::string toCharcode(const std::string &s){
     std::string r;
     for(size_t i=0;i<s.size();i++){if(i)r+=',';r+=std::to_string((unsigned char)s[i]);}
     return r;
 }
 static std::string toCharcodeHex(const std::string &s){
     std::string r;
     for(size_t i=0;i<s.size();i++){
         if(i)r+=",";
         std::ostringstream o;o<<"0x"<<std::uppercase<<std::hex<<(unsigned char)s[i];
         r+=o.str();
     }
     return r;
 }
 
 
 static std::string htmlDecode(const std::string &s){
     std::string r,tok;
     for(size_t i=0;i<s.size();i++){
         if(s[i]=='&'){
             size_t end=s.find(';',i);
             if(end!=std::string::npos){
                 std::string ent=s.substr(i+1,end-i-1);
                 if(ent=="amp")r+='&';
                 else if(ent=="lt")r+='<';
                 else if(ent=="gt")r+='>';
                 else if(ent=="quot")r+='"';
                 else if(ent=="apos")r+='\'';
                 else if(ent=="nbsp")r+=' ';
                 else if(!ent.empty()&&ent[0]=='#'){
                     try{
                         int cp=(ent[1]=='x'||ent[1]=='X')?
                             std::stoi(ent.substr(2),nullptr,16):
                             std::stoi(ent.substr(1));
                         if(cp>=0&&cp<128)r+=(char)cp;
                         else r+='?';
                     }catch(...){r+=s[i];}
                 }else{r+=s.substr(i,end-i+1);}
                 i=end;
             }else r+=s[i];
         }else r+=s[i];
     }
     return r;
 }
 
 
 static std::string urlDecode(const std::string &s){
     std::string r;
     for(size_t i=0;i<s.size();i++){
         if(s[i]=='%'&&i+2<s.size()&&isxdigit(s[i+1])&&isxdigit(s[i+2])){
             char hex[3]={s[i+1],s[i+2],0};
             r+=(char)std::stoi(hex,nullptr,16);i+=2;
         }else if(s[i]=='+')r+=' ';
         else r+=s[i];
     }
     return r;
 }
 
 
 static std::string base64Dec(const std::string &s){
     static const std::string chars="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
     std::string in;
     for(char c:s){if(c!='='&&c!='\n'&&c!='\r'&&c!=' '){auto p=chars.find(c);if(p!=std::string::npos)in+=c;}}
     std::string r;
     int val=0,valb=-8;
     for(unsigned char c:in){
         val=(val<<6)+chars.find(c);valb+=6;
         if(valb>=0){r+=(char)((val>>valb)&0xFF);valb-=8;}
     }
     return r;
 }
 
 
 static std::string base32Dec(const std::string &s){
     const std::string a="ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
     std::string r; int buf=0,bits=0;
     for(char c:s){
         if(c=='=')break;
         size_t p=a.find(toupper(c));
         if(p==std::string::npos)continue;
         buf=(buf<<5)|(int)p;bits+=5;
         if(bits>=8){bits-=8;r+=(char)((buf>>bits)&0xFF);}
     }
     return r;
 }
 
 
 static std::string swapEndian32(const std::string &s){
     
     std::string h;
     for(char c:s)if(isxdigit(c))h+=toupper(c);
     while(h.size()%8)h='0'+h;
     std::string r;
     for(size_t i=0;i<h.size();i+=8){
         std::string word=h.substr(i,8);
         r+=word.substr(6,2)+word.substr(4,2)+word.substr(2,2)+word.substr(0,2);
         if(i+8<h.size())r+=' ';
     }
     return r;
 }
 
 
 static std::string fromHex(const std::string &s){
     std::string h;
     for(char c:s)if(isxdigit(c))h+=c;
     if(h.size()%2)h='0'+h;
     std::string r;
     for(size_t i=0;i<h.size();i+=2){
         char byte=(char)std::stoi(h.substr(i,2),nullptr,16);
         if(byte>=32&&byte<127)r+=byte;
         else{std::ostringstream o;o<<"\\x"<<std::hex<<std::setw(2)<<std::setfill('0')<<(unsigned char)byte;r+=o.str();}
     }
     return r;
 }
 
 
 static std::string fromHexDump(const std::string &s){
     std::string r;
     for(size_t i=0;i<s.size();i++){
         
         
         if(i+1<s.size()&&isxdigit(s[i])&&isxdigit(s[i+1])){
             
             char byte=(char)std::stoi(s.substr(i,2),nullptr,16);
             if(byte>=32&&byte<127)r+=byte; else r+='.';
             i++;
         }
     }
     return r;
 }
 
 
 static std::string fromBinary(const std::string &s){
     std::string bits,r;
     for(char c:s)if(c=='0'||c=='1')bits+=c;
     while(bits.size()%8)bits='0'+bits;
     for(size_t i=0;i<bits.size();i+=8){
         char byte=0;
         for(int b=0;b<8;b++)byte=(byte<<1)|(bits[i+b]-'0');
         if(byte>=32&&byte<127)r+=byte;
         else{std::ostringstream o;o<<"\\x"<<std::hex<<std::setw(2)<<std::setfill('0')<<(unsigned char)byte;r+=o.str();}
     }
     return r;
 }
 
 
 static std::string fromOctal(const std::string &s){
     std::string r; std::istringstream iss(s); std::string tok;
     while(iss>>tok){
         
         if(tok.size()>1&&tok[0]=='\\'&&tok[1]=='0')tok=tok.substr(2);
         else if(tok[0]=='0'&&tok.size()>1)tok=tok.substr(1);
         try{int v=std::stoi(tok,nullptr,8);if(v>=0&&v<256)r+=(char)v;}catch(...){}
     }
     return r.empty()?"(no valid octals)":r;
 }
 
 
 static std::string fromDecimal(const std::string &s){
     std::string r; std::istringstream iss(s); std::string tok;
     while(iss>>tok){
         
         std::string clean;for(char c:tok)if(isdigit(c))clean+=c;
         try{int v=std::stoi(clean);if(v>=0&&v<256)r+=(char)v;}catch(...){}
     }
     return r.empty()?"(no valid decimals)":r;
 }
 
 
 static std::string fromUnixTimestamp(const std::string &s){
     long long ts=0;
     try{ts=std::stoll(s);}catch(...){return "need numeric timestamp";}
     time_t t=(time_t)ts;
     char buf[64];
     struct tm *tm=gmtime(&t);
     strftime(buf,sizeof(buf),"%Y-%m-%d %H:%M:%S UTC",tm);
     return std::string(buf)+" (UNIX="+std::to_string(ts)+")";
 }
 
 static std::string toUnixTimestamp(const std::string &s){
     
     try{
         long long v=std::stoll(s);
         return std::to_string(v)+" (already numeric)";
     }catch(...){}
     return "(pass a date string — complex parsing not supported in terminal mode)";
 }
 
 
 static std::string defangURL(const std::string &s){
     std::string r=s;
     
     std::string out;
     for(size_t i=0;i<r.size();i++){
         if(r[i]=='.'&&i>0&&(isalnum(r[i-1])||r[i-1]==']')){out+="[.]";}
         else if(i+3<r.size()&&r.substr(i,3)=="://"){out+="[://]";i+=2;}
         else out+=r[i];
     }
     return out;
 }
 
 static std::string refangURL(const std::string &s){
     std::string r=s,out;
     
     for(size_t i=0;i<r.size();i++){
         if(i+2<r.size()&&r.substr(i,3)=="[.]"){out+='.';i+=2;}
         else if(i+4<r.size()&&r.substr(i,5)=="[://]"){out+="://";i+=4;}
         else out+=r[i];
     }
     return out;
 }
 
 
 static std::string escapeString(const std::string &s){
     std::string r;
     for(unsigned char c:s){
         switch(c){
             case '\\':r+="\\\\";break;
             case '"':r+="\\\"";break;
             case '\'':r+="\\'";break;
             case '\n':r+="\\n";break;
             case '\r':r+="\\r";break;
             case '\t':r+="\\t";break;
             case '\0':r+="\\0";break;
             default:if(c<32||c>126){std::ostringstream o;o<<"\\x"<<std::hex<<std::setw(2)<<std::setfill('0')<<(int)c;r+=o.str();}else r+=c;
         }
     }
     return r;
 }
 static std::string unescapeString(const std::string &s){
     std::string r;
     for(size_t i=0;i<s.size();i++){
         if(s[i]=='\\'&&i+1<s.size()){
             i++;
             switch(s[i]){
                 case'n':r+='\n';break;case'r':r+='\r';break;case't':r+='\t';break;
                 case'0':r+='\0';break;case'\\':r+='\\';break;case'"':r+='"';break;
                 case'\'':r+='\'';break;
                 case'x':if(i+2<s.size()&&isxdigit(s[i+1])&&isxdigit(s[i+2])){
                     r+=(char)std::stoi(s.substr(i+1,2),nullptr,16);i+=2;}break;
                 default:r+='\\';r+=s[i];
             }
         }else r+=s[i];
     }
     return r;
 }
 
 
 static std::string stripHTML(const std::string &s){
     std::string r; bool inTag=false;
     for(char c:s){if(c=='<')inTag=true;else if(c=='>')inTag=false;else if(!inTag)r+=c;}
     return r;
 }
 
 
 static std::string extractURLs(const std::string &s){
     std::string r;
     size_t i=0;
     while(i<s.size()){
         size_t h=s.find("http",i);
         if(h==std::string::npos)break;
         size_t end=h;
         while(end<s.size()&&!isspace(s[end])&&s[end]!='<'&&s[end]!='>'&&s[end]!='"'&&s[end]!='\'')end++;
         if(!r.empty())r+='\n';
         r+=s.substr(h,end-h);
         i=end;
     }
     return r.empty()?"(no URLs found)":r;
 }
 
 
 static std::string extractIPs(const std::string &s){
     std::string r;
     
     for(size_t i=0;i<s.size();i++){
         if(isdigit(s[i])){
             size_t j=i;
             int dots=0,nums=0;
             while(j<s.size()&&(isdigit(s[j])||s[j]=='.')){if(s[j]=='.')dots++;else nums++;j++;}
             if(dots==3&&nums>=4&&nums<=12){
                 if(!r.empty())r+='\n';
                 r+=s.substr(i,j-i);
             }
             i=j;
         }
     }
     return r.empty()?"(no IPs found)":r;
 }
 
 
 static std::string extractEmails(const std::string &s){
     std::string r;
     for(size_t i=0;i<s.size();i++){
         if(isalnum(s[i])||s[i]=='.'||s[i]=='_'||s[i]=='-'||s[i]=='+'){
             size_t start=i;
             while(i<s.size()&&(isalnum(s[i])||s[i]=='.'||s[i]=='_'||s[i]=='-'||s[i]=='+'))i++;
             if(i<s.size()&&s[i]=='@'){
                 size_t atpos=i++;
                 while(i<s.size()&&(isalnum(s[i])||s[i]=='.'||s[i]=='-'))i++;
                 if(i>atpos+2){if(!r.empty())r+='\n';r+=s.substr(start,i-start);}
             }
             i--;
         }
     }
     return r.empty()?"(no emails found)":r;
 }
 
 
 
 
 static std::string removeWhitespace(const std::string &s){
     std::string r;for(char c:s)if(!isspace(c))r+=c;return r;
 }
 static std::string removeLinebreaks(const std::string &s){
     std::string r;for(char c:s)if(c!='\n'&&c!='\r')r+=c;return r;
 }
 static std::string removeNullBytes(const std::string &s){
     std::string r;for(char c:s)if(c!='\0')r+=c;return r;
 }
 
 
 static std::string trimStr(const std::string &s){
     size_t a=s.find_first_not_of(" \t\n\r");
     size_t b=s.find_last_not_of(" \t\n\r");
     if(a==std::string::npos)return "";
     return s.substr(a,b-a+1);
 }
 
 
 static std::string headLines(const std::string &s){
     std::istringstream iss(s); std::string line,r; int n=0;
     while(std::getline(iss,line)&&n<10){if(!r.empty())r+='\n';r+=line;n++;}
     if(n==10)r+="\n... (truncated at 10 lines)";
     return r.empty()?"(empty)":r;
 }
 static std::string tailLines(const std::string &s){
     std::vector<std::string> lines;
     std::istringstream iss(s); std::string line;
     while(std::getline(iss,line))lines.push_back(line);
     std::string r;
     size_t start=lines.size()>10?lines.size()-10:0;
     for(size_t i=start;i<lines.size();i++){if(!r.empty())r+='\n';r+=lines[i];}
     return r.empty()?"(empty)":r;
 }
 static std::string sortLines(const std::string &s){
     std::vector<std::string> lines;
     std::istringstream iss(s); std::string line;
     while(std::getline(iss,line))lines.push_back(line);
     std::sort(lines.begin(),lines.end());
     std::string r;
     for(auto&l:lines){if(!r.empty())r+='\n';r+=l;}
     return r;
 }
 static std::string uniqueLines(const std::string &s){
     std::vector<std::string> lines;
     std::istringstream iss(s); std::string line;
     while(std::getline(iss,line))lines.push_back(line);
     std::sort(lines.begin(),lines.end());
     lines.erase(std::unique(lines.begin(),lines.end()),lines.end());
     std::string r;
     for(auto&l:lines){if(!r.empty())r+='\n';r+=l;}
     return r;
 }
 static std::string countLines(const std::string &s){
     int n=1;for(char c:s)if(c=='\n')n++;
     return std::to_string(n)+" lines, "+std::to_string(s.size())+" bytes";
 }
 
 
 static std::string padStart(const std::string &s){
     int target=32;
     if((int)s.size()>=target)return s;
     return std::string(target-s.size(),' ')+s;
 }
 static std::string padEnd(const std::string &s){
     int target=32;
     if((int)s.size()>=target)return s;
     return s+std::string(target-s.size(),' ');
 }
 
 
 static std::string convertDataUnits(const std::string &s){
     double v=0;
     try{v=std::stod(s);}catch(...){return "need numeric input (bytes)";}
     std::ostringstream o;
     o<<std::fixed<<std::setprecision(4);
     o<<"Bytes: "<<v<<"\n"
      <<"KiB:   "<<v/1024<<"\n"
      <<"MiB:   "<<v/(1024*1024)<<"\n"
      <<"GiB:   "<<v/(1024.0*1024*1024)<<"\n"
      <<"KB:    "<<v/1000<<"\n"
      <<"MB:    "<<v/1000000<<"\n"
      <<"GB:    "<<v/1000000000.0;
     return o.str();
 }
 
 
 static std::string indexOfCoincidence(const std::string &s){
     int freq[26]={};int n=0;
     for(char c:s)if(isalpha(c)){freq[tolower(c)-'a']++;n++;}
     if(n<2)return "(need at least 2 alpha chars)";
     double ic=0;
     for(int i=0;i<26;i++)ic+=(double)freq[i]*(freq[i]-1);
     ic/=(double)n*(n-1);
     std::ostringstream o;
     o<<std::fixed<<std::setprecision(6)<<ic
      <<" (English≈0.0667, random≈0.0385)";
     return o.str();
 }
 
 
 static std::string chiSquared(const std::string &s){
     
     static double eng[]={8.2,1.5,2.8,4.3,13.0,2.2,2.0,6.1,7.0,0.15,
                          0.77,4.0,2.4,6.7,7.5,1.9,0.095,6.0,6.3,9.1,
                          2.8,0.98,2.4,0.15,2.0,0.074};
     int freq[26]={};int n=0;
     for(char c:s)if(isalpha(c)){freq[tolower(c)-'a']++;n++;}
     if(n<2)return "(need at least 2 alpha chars)";
     double chi=0;
     for(int i=0;i<26;i++){
         double expected=(eng[i]/100.0)*n;
         if(expected>0)chi+=(freq[i]-expected)*(freq[i]-expected)/expected;
     }
     std::ostringstream o;
     o<<std::fixed<<std::setprecision(4)<<chi
      <<" (lower = closer to English; <100 likely English text)";
     return o.str();
 }
 
 
 static std::string enigmaSimple(const std::string &s){
     
     static const char* rotors[]={"EKMFLGDQVZNTOWYHXUSPAIBRCJ",
                                   "AJDKSIRUXBLHWTMCQGZNPYFVOE",
                                   "BDFHJLCPRTXVZNYEIWGAKMUSQO"};
     static const char* reflect="YRUHQSLDPXNGOKMIEBFZCWVJAT";
     static const int notch[]={16,4,21}; 
     int pos[]={0,0,0};
     std::string r;
     for(char c:s){
         if(!isalpha(c)){r+=c;continue;}
         c=toupper(c);
         
         bool step2=true; 
         if(pos[0]==notch[0])step2=true;
         if(pos[1]==notch[1]){pos[1]=(pos[1]+1)%26;pos[2]=(pos[2]+1)%26;}
         if(step2)pos[0]=(pos[0]+1)%26;
         
         int idx=c-'A';
         for(int i=0;i<3;i++) idx=(rotors[i][(idx+pos[i])%26]-'A'-pos[i]+26)%26;
         idx=reflect[idx]-'A';
         for(int i=2;i>=0;i--) {
             const char*r2=rotors[i];
             for(int j=0;j<26;j++)if((r2[(j+pos[i])%26]-'A'-pos[i]+26)%26==idx){idx=j;break;}
         }
         r+=(char)('A'+idx);
     }
     return r+" [Enigma M3, rotors I-II-III, reflector B, all start pos A]";
 }
 
 
 static std::string caesarBruteForce(const std::string &s){
     std::string r;
     for(int n=1;n<26;n++){
         std::string line="ROT-"+std::to_string(n)+": ";
         for(char c:s){
             if(isalpha(c)){char b=isupper(c)?'A':'a';line+=(char)(b+(c-b+n)%26);}
             else line+=c;
         }
         r+=line+'\n';
     }
     return r;
 }
 
 
 static std::string xorBruteForce(const std::string &s){
     std::string best;int bestKey=0;double bestScore=-1;
     static double eng[]={8.2,1.5,2.8,4.3,13.0,2.2,2.0,6.1,7.0,0.15,0.77,4.0,2.4,6.7,7.5,1.9,0.095,6.0,6.3,9.1,2.8,0.98,2.4,0.15,2.0,0.074};
     for(int k=1;k<256;k++){
         std::string cand;double score=0;
         for(unsigned char c:s){char d=c^k;cand+=d;if(isalpha(d))score+=eng[tolower(d)-'a'];}
         if(score>bestScore){bestScore=score;bestKey=k;best=cand;}
     }
     std::ostringstream o;
     o<<"Best key: 0x"<<std::uppercase<<std::hex<<std::setw(2)<<std::setfill('0')<<bestKey
      <<" (score="<<std::fixed<<std::setprecision(1)<<bestScore<<")\n"<<best;
     return o.str();
 }
 
 
 static std::string add(const std::string &s){
     long long v=0;
     try{
         auto&ss=s;
         if(ss.find(' ')!=std::string::npos){
             std::istringstream iss(ss); long long a,b; iss>>a>>b; v=a+b;
         }else v=std::stoll(ss)+1;
     }catch(...){return "need: 'a b' or single number";}
     std::ostringstream o;
     o<<"dec="<<v<<" hex=0x"<<std::uppercase<<std::hex<<v<<" bin=";
     if(v<=0)o<<"0";else{std::string b;long long t=v;while(t){b=(char)('0'+(t&1))+b;t>>=1;}o<<b;}
     return o.str();
 }
 static std::string subtract(const std::string &s){
     try{
         std::istringstream iss(s); long long a,b; iss>>a>>b;
         long long v=a-b;
         std::ostringstream o;
         o<<"dec="<<v<<" hex=0x"<<std::uppercase<<std::hex<<v;
         return o.str();
     }catch(...){return "need: 'a b'";}
 }
 static std::string multiply(const std::string &s){
     try{
         std::istringstream iss(s); long long a,b; iss>>a>>b;
         long long v=a*b;
         std::ostringstream o;
         o<<"dec="<<v<<" hex=0x"<<std::uppercase<<std::hex<<v;
         return o.str();
     }catch(...){return "need: 'a b'";}
 }
 static std::string divideOp(const std::string &s){
     try{
         std::istringstream iss(s); double a,b; iss>>a>>b;
         if(b==0)return "division by zero";
         std::ostringstream o;o<<std::fixed<<std::setprecision(6)<<a/b;
         return o.str();
     }catch(...){return "need: 'a b'";}
 }
 static std::string moduloOp(const std::string &s){
     try{
         std::istringstream iss(s); long long a,b; iss>>a>>b;
         if(b==0)return "division by zero";
         return std::to_string(a%b);
     }catch(...){return "need: 'a b'";}
 }
 static std::string powerOp(const std::string &s){
     try{
         std::istringstream iss(s); double a,b; iss>>a>>b;
         std::ostringstream o;o<<std::fixed<<std::setprecision(6)<<pow(a,b);
         return o.str();
     }catch(...){return "need: 'a b'";}
 }
 static std::string sqrtOp(const std::string &s){
     double v=0;
     try{v=std::stod(s);}catch(...){return "need numeric input";}
     if(v<0)return "imaginary: sqrt("+s+") = "+std::to_string(sqrt(-v))+"i";
     std::ostringstream o;o<<std::fixed<<std::setprecision(10)<<sqrt(v);
     return o.str();
 }
 static std::string logOp(const std::string &s){
     double v=0;
     try{v=std::stod(s);}catch(...){return "need numeric input";}
     if(v<=0)return "undefined (log of non-positive)";
     std::ostringstream o;
     o<<std::fixed<<std::setprecision(10)
      <<"ln: "<<log(v)<<"  log2: "<<log2(v)<<"  log10: "<<log10(v);
     return o.str();
 }
 
 
 static std::string bitwiseAND(const std::string &s){
     std::istringstream iss(s);long long a=0,b=0;
     try{iss>>a>>b;}catch(...){return "need: 'a b'";}
     long long v=a&b;
     std::ostringstream o;o<<"dec="<<v<<" hex=0x"<<std::uppercase<<std::hex<<v;return o.str();
 }
 static std::string bitwiseOR(const std::string &s){
     std::istringstream iss(s);long long a=0,b=0;
     try{iss>>a>>b;}catch(...){return "need: 'a b'";}
     long long v=a|b;
     std::ostringstream o;o<<"dec="<<v<<" hex=0x"<<std::uppercase<<std::hex<<v;return o.str();
 }
 static std::string bitwiseNOT(const std::string &s){
     long long v=0;try{v=std::stoll(s);}catch(...){return "need numeric";}
     std::ostringstream o;o<<"dec="<<~v<<" hex=0x"<<std::uppercase<<std::hex<<(~v&0xFFFFFFFF);return o.str();
 }
 static std::string shiftLeft(const std::string &s){
     std::istringstream iss(s);long long a=0,n=1;iss>>a>>n;
     std::ostringstream o;o<<"dec="<<(a<<n)<<" hex=0x"<<std::uppercase<<std::hex<<(a<<n);return o.str();
 }
 static std::string shiftRight(const std::string &s){
     std::istringstream iss(s);long long a=0,n=1;iss>>a>>n;
     std::ostringstream o;o<<"dec="<<(a>>n)<<" hex=0x"<<std::uppercase<<std::hex<<(a>>n);return o.str();
 }
 
 
 static std::string toHexNL(const std::string &s){
     std::string r;
     for(size_t i=0;i<s.size();i++){
         if(i&&i%16==0)r+='\n';
         else if(i)r+=' ';
         std::ostringstream o;
         o<<std::uppercase<<std::hex<<std::setw(2)<<std::setfill('0')<<(unsigned char)s[i];
         r+=o.str();
     }
     return r;
 }
 
 
 
 static std::string sha1(const std::string &msg){
     uint32_t h[]={0x67452301,0xEFCDAB89,0x98BADCFE,0x10325476,0xC3D2E1F0};
     auto leftrot=[](uint32_t x,uint32_t n){return(x<<n)|(x>>(32-n));};
     std::vector<uint8_t> data(msg.begin(),msg.end());
     uint64_t origLen=data.size()*8;
     data.push_back(0x80);
     while(data.size()%64!=56)data.push_back(0);
     for(int i=7;i>=0;i--)data.push_back((origLen>>(i*8))&0xFF);
     for(size_t i=0;i<data.size();i+=64){
         uint32_t w[80]={};
         for(int j=0;j<16;j++)
             w[j]=(data[i+j*4]<<24)|(data[i+j*4+1]<<16)|(data[i+j*4+2]<<8)|data[i+j*4+3];
         for(int j=16;j<80;j++)w[j]=leftrot(w[j-3]^w[j-8]^w[j-14]^w[j-16],1);
         uint32_t a=h[0],b=h[1],c=h[2],d=h[3],e=h[4];
         for(int j=0;j<80;j++){
             uint32_t f,k;
             if(j<20){f=(b&c)|((~b)&d);k=0x5A827999;}
             else if(j<40){f=b^c^d;k=0x6ED9EBA1;}
             else if(j<60){f=(b&c)|(b&d)|(c&d);k=0x8F1BBCDC;}
             else{f=b^c^d;k=0xCA62C1D6;}
             uint32_t tmp=leftrot(a,5)+f+e+k+w[j];
             e=d;d=c;c=leftrot(b,30);b=a;a=tmp;
         }
         h[0]+=a;h[1]+=b;h[2]+=c;h[3]+=d;h[4]+=e;
     }
     std::ostringstream o;
     for(int i=0;i<5;i++)o<<std::hex<<std::setw(8)<<std::setfill('0')<<h[i];
     return o.str();
 }
 
 
 static std::string sha256(const std::string &msg){
     static const uint32_t K[]={
         0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
         0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
         0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
         0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
         0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
         0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
         0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
         0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
     };
     auto rotr=[](uint32_t x,int n){return(x>>n)|(x<<(32-n));};
     uint32_t h[]={0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19};
     std::vector<uint8_t> data(msg.begin(),msg.end());
     uint64_t origLen=data.size()*8;
     data.push_back(0x80);
     while(data.size()%64!=56)data.push_back(0);
     for(int i=7;i>=0;i--)data.push_back((origLen>>(i*8))&0xFF);
     for(size_t i=0;i<data.size();i+=64){
         uint32_t w[64]={};
         for(int j=0;j<16;j++)
             w[j]=(data[i+j*4]<<24)|(data[i+j*4+1]<<16)|(data[i+j*4+2]<<8)|data[i+j*4+3];
         for(int j=16;j<64;j++){
             uint32_t s0=rotr(w[j-15],7)^rotr(w[j-15],18)^(w[j-15]>>3);
             uint32_t s1=rotr(w[j-2],17)^rotr(w[j-2],19)^(w[j-2]>>10);
             w[j]=w[j-16]+s0+w[j-7]+s1;
         }
         uint32_t a=h[0],b=h[1],c=h[2],d=h[3],e=h[4],f=h[5],g=h[6],hh=h[7];
         for(int j=0;j<64;j++){
             uint32_t S1=rotr(e,6)^rotr(e,11)^rotr(e,25);
             uint32_t ch=(e&f)^((~e)&g);
             uint32_t t1=hh+S1+ch+K[j]+w[j];
             uint32_t S0=rotr(a,2)^rotr(a,13)^rotr(a,22);
             uint32_t maj=(a&b)^(a&c)^(b&c);
             uint32_t t2=S0+maj;
             hh=g;g=f;f=e;e=d+t1;d=c;c=b;b=a;a=t1+t2;
         }
         h[0]+=a;h[1]+=b;h[2]+=c;h[3]+=d;h[4]+=e;h[5]+=f;h[6]+=g;h[7]+=hh;
     }
     std::ostringstream o;
     for(int i=0;i<8;i++)o<<std::hex<<std::setw(8)<<std::setfill('0')<<h[i];
     return o.str();
 }
 
 
 static std::string md5(const std::string &msg){
     static const uint32_t K[]={
         0xd76aa478,0xe8c7b756,0x242070db,0xc1bdceee,0xf57c0faf,0x4787c62a,0xa8304613,0xfd469501,
         0x698098d8,0x8b44f7af,0xffff5bb1,0x895cd7be,0x6b901122,0xfd987193,0xa679438e,0x49b40821,
         0xf61e2562,0xc040b340,0x265e5a51,0xe9b6c7aa,0xd62f105d,0x02441453,0xd8a1e681,0xe7d3fbc8,
         0x21e1cde6,0xc33707d6,0xf4d50d87,0x455a14ed,0xa9e3e905,0xfcefa3f8,0x676f02d9,0x8d2a4c8a,
         0xfffa3942,0x8771f681,0x6d9d6122,0xfde5380c,0xa4beea44,0x4bdecfa9,0xf6bb4b60,0xbebfbc70,
         0x289b7ec6,0xeaa127fa,0xd4ef3085,0x04881d05,0xd9d4d039,0xe6db99e5,0x1fa27cf8,0xc4ac5665,
         0xf4292244,0x432aff97,0xab9423a7,0xfc93a039,0x655b59c3,0x8f0ccc92,0xffeff47d,0x85845dd1,
         0x6fa87e4f,0xfe2ce6e0,0xa3014314,0x4e0811a1,0xf7537e82,0xbd3af235,0x2ad7d2bb,0xeb86d391
     };
     static const uint32_t S[]={
         7,12,17,22,7,12,17,22,7,12,17,22,7,12,17,22,
         5,9,14,20,5,9,14,20,5,9,14,20,5,9,14,20,
         4,11,16,23,4,11,16,23,4,11,16,23,4,11,16,23,
         6,10,15,21,6,10,15,21,6,10,15,21,6,10,15,21
     };
     auto leftrot=[](uint32_t x,uint32_t n){return(x<<n)|(x>>(32-n));};
     std::vector<uint8_t> data(msg.begin(),msg.end());
     uint64_t origLen=data.size()*8;
     data.push_back(0x80);
     while(data.size()%64!=56)data.push_back(0);
     for(int i=0;i<8;i++)data.push_back((origLen>>(i*8))&0xFF); 
     uint32_t a=0x67452301,b=0xEFCDAB89,c=0x98BADCFE,d=0x10325476;
     for(size_t i=0;i<data.size();i+=64){
         uint32_t M[16]={};
         for(int j=0;j<16;j++)
             M[j]=data[i+j*4]|(data[i+j*4+1]<<8)|(data[i+j*4+2]<<16)|(data[i+j*4+3]<<24);
         uint32_t A=a,B=b,C=c,D=d;
         for(int j=0;j<64;j++){
             uint32_t F2,g;
             if(j<16){F2=(B&C)|((~B)&D);g=j;}
             else if(j<32){F2=(D&B)|((~D)&C);g=(5*j+1)%16;}
             else if(j<48){F2=B^C^D;g=(3*j+5)%16;}
             else{F2=C^(B|(~D));g=(7*j)%16;}
             F2+=A+K[j]+M[g];
             A=D;D=C;C=B;B+=leftrot(F2,S[j]);
         }
         a+=A;b+=B;c+=C;d+=D;
     }
     auto le=[](uint32_t x){std::ostringstream o;
         for(int i=0;i<4;i++)o<<std::hex<<std::setw(2)<<std::setfill('0')<<((x>>(i*8))&0xFF);
         return o.str();};
     return le(a)+le(b)+le(c)+le(d);
 }
 
 
 static std::string hmacSha256(const std::string &s){
     
     size_t sp=s.find(' ');
     std::string key=(sp==std::string::npos)?"key":s.substr(0,sp);
     std::string msg=(sp==std::string::npos)?s:s.substr(sp+1);
     
     std::string kp=key;
     if(kp.size()>64)kp=sha256(kp); 
     while(kp.size()<64)kp+='\0';
     std::string ipad,opad;
     for(char c:kp){ipad+=(char)(c^0x36);opad+=(char)(c^0x5c);}
     std::string inner=sha256(ipad+msg);
     
     std::string innerBytes;
     for(size_t i=0;i<inner.size();i+=2)
         innerBytes+=(char)std::stoi(inner.substr(i,2),nullptr,16);
     return "HMAC-SHA256(key=\""+key+"\", msg): "+sha256(opad+innerBytes);
 }
 
 
 static std::string parseIPv4(const std::string &s){
     std::string ip=trimStr(s);
     uint32_t a=0,b=0,c=0,d=0;
     if(sscanf(ip.c_str(),"%u.%u.%u.%u",&a,&b,&c,&d)!=4)return "invalid IPv4: "+ip;
     if(a>255||b>255||c>255||d>255)return "octet out of range";
     uint32_t v=(a<<24)|(b<<16)|(c<<8)|d;
     std::ostringstream o;
     o<<"Decimal:  "<<v<<"\n"
      <<"Hex:      0x"<<std::uppercase<<std::hex<<std::setw(8)<<std::setfill('0')<<v<<"\n"
      <<"Binary:   ";
     for(int i=31;i>=0;i--){o<<((v>>i)&1);if(i%8==0&&i)o<<'.';}
     o<<"\nClass:    ";
     if(a<128)o<<"A";else if(a<192)o<<"B";else if(a<224)o<<"C";else if(a<240)o<<"D (multicast)";else o<<"E (reserved)";
     o<<"\nPrivate:  ";
     bool priv=(a==10)||(a==172&&b>=16&&b<=31)||(a==192&&b==168);
     o<<(priv?"Yes":"No");
     o<<"\nLoopback: "<<(a==127?"Yes":"No");
     return o.str();
 }
 
 
 static std::string parseIPv6(const std::string &s){
     std::string ip=trimStr(s);
     
     std::ostringstream o;
     o<<"Input:    "<<ip<<"\n";
     
     size_t dc=ip.find("::");
     std::string expanded=ip;
     if(dc!=std::string::npos){
         
         int existing=0;for(char c:ip)if(c==':')existing++;
         if(ip.back()==':')existing--;
         int needed=7-existing;
         std::string fill;for(int i=0;i<needed;i++)fill+=":0";
         expanded.replace(dc,2,fill+":");
     }
     o<<"Expanded: "<<expanded;
     return o.str();
 }
 
 
 static std::string parseCIDR(const std::string &s){
     std::string ip; int prefix=24;
     size_t slash=s.find('/');
     if(slash==std::string::npos){ip=s;}
     else{ip=s.substr(0,slash);try{prefix=std::stoi(s.substr(slash+1));}catch(...){}}
     uint32_t a=0,b=0,c=0,d=0;
     if(sscanf(ip.c_str(),"%u.%u.%u.%u",&a,&b,&c,&d)!=4)return "invalid CIDR: "+s;
     uint32_t addr=(a<<24)|(b<<16)|(c<<8)|d;
     uint32_t mask=prefix>0?(0xFFFFFFFF<<(32-prefix)):0;
     uint32_t net=addr&mask;
     uint32_t bcast=net|(~mask);
     uint32_t hosts=bcast-net-1;
     auto fmt=[](uint32_t v){
         return std::to_string(v>>24)+"."+std::to_string((v>>16)&0xFF)+"."+std::to_string((v>>8)&0xFF)+"."+std::to_string(v&0xFF);
     };
     std::ostringstream o;
     o<<"Network:   "<<fmt(net)<<"/"<<prefix<<"\n"
      <<"Mask:      "<<fmt(mask)<<"\n"
      <<"First host:"<<fmt(net+1)<<"\n"
      <<"Last host: "<<fmt(bcast-1)<<"\n"
      <<"Broadcast: "<<fmt(bcast)<<"\n"
      <<"Hosts:     "<<hosts;
     return o.str();
 }
 
 
 static std::string jwtDecode(const std::string &s){
     
     std::vector<std::string> parts;
     std::string cur;
     for(char c:s){if(c=='.'){{parts.push_back(cur);cur="";}}else cur+=c;}
     parts.push_back(cur);
     if(parts.size()<2)return "invalid JWT (need at least 2 parts)";
     auto b64urlDec=[](std::string in)->std::string{
         
         while(in.size()%4)in+='=';
         for(char&c:in){if(c=='-')c='+';if(c=='_')c='/';}
         return base64Dec(in);
     };
     std::string header=b64urlDec(parts[0]);
     std::string payload=b64urlDec(parts[1]);
     return "Header:\n"+header+"\n\nPayload:\n"+payload+
            "\n\nSignature: "+parts[2].substr(0,20)+"... (not verified)";
 }
 
 
 static std::string decodeProtobuf(const std::string &s){
     
     std::string r;
     size_t i=0;
     int fieldCount=0;
     while(i<s.size()&&fieldCount<32){
         
         uint64_t key=0; int shift=0;
         while(i<s.size()){
             uint8_t b=(unsigned char)s[i++];
             key|=(uint64_t)(b&0x7F)<<shift;
             if(!(b&0x80))break;
             shift+=7;
         }
         int fieldNum=(int)(key>>3);
         int wireType=(int)(key&0x7);
         if(!r.empty())r+='\n';
         r+="Field "+std::to_string(fieldNum)+", wire="+std::to_string(wireType)+": ";
         if(wireType==0){ 
             uint64_t v=0;shift=0;
             while(i<s.size()){uint8_t b=(unsigned char)s[i++];v|=(uint64_t)(b&0x7F)<<shift;if(!(b&0x80))break;shift+=7;}
             r+=std::to_string(v);
         }else if(wireType==2){ 
             uint64_t len=0;shift=0;
             while(i<s.size()){uint8_t b=(unsigned char)s[i++];len|=(uint64_t)(b&0x7F)<<shift;if(!(b&0x80))break;shift+=7;}
             std::string val;
             for(uint64_t j=0;j<len&&i<s.size();j++,i++){
                 unsigned char c=(unsigned char)s[i];
                 if(c>=32&&c<127)val+=c; else{std::ostringstream o2;o2<<"\\x"<<std::hex<<std::setw(2)<<std::setfill('0')<<(int)c;val+=o2.str();}
             }
             r+="\""+val+"\" ("+std::to_string(len)+" bytes)";
         }else if(wireType==5){ 
             if(i+4<=s.size()){
                 uint32_t v=0;for(int k=0;k<4;k++)v|=(unsigned char)s[i+k]<<(k*8);
                 std::ostringstream o2;o2<<"0x"<<std::uppercase<<std::hex<<v;r+=o2.str();i+=4;
             }
         }else if(wireType==1){ 
             if(i+8<=s.size()){
                 uint64_t v=0;for(int k=0;k<8;k++)v|=(uint64_t)(unsigned char)s[i+k]<<(k*8);
                 r+=std::to_string(v);i+=8;
             }
         }else{r+="(unsupported wire type)";break;}
         fieldCount++;
     }
     return r.empty()?"(no protobuf fields decoded)":r;
 }
 
 
 static std::string jsonPrettify(const std::string &s){
     std::string r; int indent=0; bool inStr=false;
     for(size_t i=0;i<s.size();i++){
         char c=s[i];
         if(c=='"'&&(i==0||s[i-1]!='\\'))inStr=!inStr;
         if(inStr){r+=c;continue;}
         if(c=='{'){r+=c;r+='\n';indent++;for(int j=0;j<indent*2;j++)r+=' ';}
         else if(c=='}'){r+='\n';indent--;for(int j=0;j<indent*2;j++)r+=' ';r+=c;}
         else if(c=='['){r+=c;if(i+1<s.size()&&s[i+1]!=']'){r+='\n';indent++;for(int j=0;j<indent*2;j++)r+=' ';}}
         else if(c==']'){if(i>0&&s[i-1]!='['){r+='\n';indent--;for(int j=0;j<indent*2;j++)r+=' ';}r+=c;}
         else if(c==','){r+=c;r+='\n';for(int j=0;j<indent*2;j++)r+=' ';}
         else if(c==':'){r+=c;r+=' ';}
         else if(c!=' '&&c!='\t'&&c!='\n'&&c!='\r')r+=c;
     }
     return r;
 }
 static std::string jsonMinify(const std::string &s){
     std::string r; bool inStr=false;
     for(size_t i=0;i<s.size();i++){
         char c=s[i];
         if(c=='"'&&(i==0||s[i-1]!='\\'))inStr=!inStr;
         if(!inStr&&(c==' '||c=='\t'||c=='\n'||c=='\r'))continue;
         r+=c;
     }
     return r;
 }
 
 
 static std::string parseDateTime(const std::string &s){
     
     try{
         long long ts=std::stoll(s);
         time_t t=(time_t)ts;
         char buf[64];struct tm*tm=gmtime(&t);
         strftime(buf,sizeof(buf),"%Y-%m-%d %H:%M:%S UTC",tm);
         return std::string(buf)+"\nUNIX: "+std::to_string(ts);
     }catch(...){}
     
     return "Input: "+s+"\n(pass a UNIX timestamp for full parsing)";
 }
 
 
 static std::string formatNumber(const std::string &s){
     std::string clean;bool neg=false;
     for(char c:s){if(c=='-'&&clean.empty())neg=true;else if(isdigit(c)||c=='.')clean+=c;}
     if(clean.empty())return "(no number found)";
     size_t dot=clean.find('.');
     std::string intPart=dot==std::string::npos?clean:clean.substr(0,dot);
     std::string decPart=dot==std::string::npos?"":clean.substr(dot);
     std::string r;
     for(int i=(int)intPart.size()-1,k=0;i>=0;i--,k++){if(k&&k%3==0)r=','+r;r=intPart[i]+r;}
     if(neg)r='-'+r;
     return r+decPart;
 }
 
 
 static std::string parseUUID(const std::string &s){
     std::string u=trimStr(s);
     
     if(!u.empty()&&u[0]=='{')u=u.substr(1);
     if(!u.empty()&&u.back()=='}')u.pop_back();
     if(u.size()<32)return "too short for UUID";
     
     std::string h;for(char c:u)if(isxdigit(c))h+=toupper(c);
     if(h.size()!=32)return "UUID must have exactly 32 hex digits, got "+std::to_string(h.size());
     int version=std::stoi(h.substr(12,1),nullptr,16);
     int variant=(std::stoi(h.substr(16,1),nullptr,16)>>2);
     std::string vstr=variant>=2?"RFC 4122":"other";
     return "UUID:    "+h.substr(0,8)+"-"+h.substr(8,4)+"-"+h.substr(12,4)+"-"+h.substr(16,4)+"-"+h.substr(20)+
            "\nVersion: "+std::to_string(version)+
            "\nVariant: "+vstr;
 }
 
 
 static std::string hexToDec(const std::string &s){
     std::string h=trimStr(s);
     if(h.size()>2&&h[0]=='0'&&(h[1]=='x'||h[1]=='X'))h=h.substr(2);
     try{
         unsigned long long v=std::stoull(h,nullptr,16);
         return std::to_string(v)+" (0x"+h+")";
     }catch(...){return "invalid hex: "+s;}
 }
 static std::string decToHex(const std::string &s){
     try{
         unsigned long long v=std::stoull(trimStr(s));
         std::ostringstream o;
         o<<"0x"<<std::uppercase<<std::hex<<v<<" (dec "<<v<<")";
         return o.str();
     }catch(...){return "invalid decimal: "+s;}
 }
 
 
 static std::string rot13WithDigits(const std::string &s){
     std::string r;
     for(char c:s){
         if(c>='a'&&c<='z')r+=(char)('a'+(c-'a'+13)%26);
         else if(c>='A'&&c<='Z')r+=(char)('A'+(c-'A'+13)%26);
         else if(c>='0'&&c<='9')r+=(char)('0'+(c-'0'+5)%10);
         else r+=c;
     }
     return r;
 }
 
 
 static std::string rabbitCipher(const std::string &s){
     
     
     uint32_t state=0xDEADBEEF;
     std::string r;
     for(unsigned char c:s){
         state^=(state<<13);state^=(state>>17);state^=(state<<5);
         r+=(char)(c^(state&0xFF));
     }
     
     return toHexStr(r," ",true)+" [RC4-like stream cipher demo; real Rabbit requires key]";
 }
 
 
 static std::string beaufort(const std::string &s){
     
     const std::string key="BEAUFORT";
     std::string r;int ki=0;
     for(char c:s){
         if(isalpha(c)){
             char u=toupper(c);
             r+=(char)('A'+(key[ki%key.size()]-u+26)%26);
             ki++;
         }else r+=c;
     }
     return r;
 }
 
 
 static std::string halfwaySubstitute(const std::string &s){
     std::string r;
     for(char c:s){
         if(isalpha(c)){
             char b=isupper(c)?'A':'a';
             int pos=c-b;
             r+=(char)(b+(pos+13)%26);
         }else r+=c;
     }
     return r;
 }
 
 
 static std::string escapeHTMLFull(const std::string &s){
     std::string r=htmlEncode(s);
     int escaped=(int)(r.size()-s.size());
     return r+"\n["+std::to_string(escaped)+" chars escaped]";
 }
 
 
 static std::string filetimeToUnix(const std::string &s){
     try{
         unsigned long long ft=std::stoull(trimStr(s));
         
         unsigned long long unix=(ft-116444736000000000ULL)/10000000ULL;
         time_t t=(time_t)unix;
         char buf[64];struct tm*tm=gmtime(&t);
         strftime(buf,sizeof(buf),"%Y-%m-%d %H:%M:%S UTC",tm);
         return "FILETIME: "+std::to_string(ft)+"\nUNIX:     "+std::to_string(unix)+"\nDate:     "+std::string(buf);
     }catch(...){return "invalid FILETIME (need 64-bit integer)";}
 }
 
 
 static std::string googleEiTimestamp(const std::string &s){
     
     try{
         std::string decoded=base64Dec(s);
         if(decoded.size()<4)return "too short";
         uint32_t ts=(unsigned char)decoded[0]|((unsigned char)decoded[1]<<8)|
                     ((unsigned char)decoded[2]<<16)|((unsigned char)decoded[3]<<24);
         time_t t=(time_t)ts;
         char buf[64];struct tm*tm=gmtime(&t);
         strftime(buf,sizeof(buf),"%Y-%m-%d %H:%M:%S UTC",tm);
         return "ei: "+s+"\nUNIX: "+std::to_string(ts)+"\nDate: "+buf;
     }catch(...){return "invalid ei parameter";}
 }
 
 
 static std::string hexToFloat(const std::string &s){
     std::string h=trimStr(s);
     if(h.size()>2&&h[0]=='0'&&(h[1]=='x'||h[1]=='X'))h=h.substr(2);
     while(h.size()<8)h='0'+h;
     try{
         uint32_t bits=std::stoul(h.substr(0,8),nullptr,16);
         float f;memcpy(&f,&bits,4);
         double d;uint64_t bits64=std::stoull(h.size()>=16?h.substr(0,16):h,nullptr,16);
         memcpy(&d,&bits64,std::min((size_t)8,h.size()/2));
         std::ostringstream o;
         o<<"float32: "<<std::fixed<<std::setprecision(10)<<f<<"\n"
          <<"bits:    0x"<<std::uppercase<<std::hex<<bits;
         return o.str();
     }catch(...){return "invalid hex";}
 }
 
 
 
 static std::string ntlmHash(const std::string &s){
     
     std::string u16;
     for(unsigned char c:s){u16+=c;u16+='\0';}
     
     auto F=[](uint32_t x,uint32_t y,uint32_t z){return(x&y)|((~x)&z);};
     auto G=[](uint32_t x,uint32_t y,uint32_t z){return(x&y)|(x&z)|(y&z);};
     auto H=[](uint32_t x,uint32_t y,uint32_t z){return x^y^z;};
     auto lr=[](uint32_t x,int n){return(x<<n)|(x>>(32-n));};
     std::vector<uint8_t> data(u16.begin(),u16.end());
     uint64_t origLen=data.size()*8;
     data.push_back(0x80);
     while(data.size()%64!=56)data.push_back(0);
     for(int i=0;i<8;i++)data.push_back((origLen>>(i*8))&0xFF);
     uint32_t a=0x67452301,b=0xEFCDAB89,c=0x98BADCFE,d=0x10325476;
     for(size_t i=0;i<data.size();i+=64){
         uint32_t X[16]={};
         for(int j=0;j<16;j++)X[j]=data[i+j*4]|(data[i+j*4+1]<<8)|(data[i+j*4+2]<<16)|(data[i+j*4+3]<<24);
         uint32_t A=a,B=b,C=c,D=d;
         int r1[]={0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
         int s1[]={3,7,11,19,3,7,11,19,3,7,11,19,3,7,11,19};
         for(int j=0;j<16;j++){uint32_t t=A+F(B,C,D)+X[r1[j]];A=D;D=C;C=B;B=lr(t,s1[j]);}
         int r2[]={0,4,8,12,1,5,9,13,2,6,10,14,3,7,11,15};
         int s2[]={3,5,9,13,3,5,9,13,3,5,9,13,3,5,9,13};
         for(int j=0;j<16;j++){uint32_t t=A+G(B,C,D)+X[r2[j]]+0x5A827999;A=D;D=C;C=B;B=lr(t,s2[j]);}
         int r3[]={0,8,4,12,2,10,6,14,1,9,5,13,3,11,7,15};
         int s3[]={3,9,11,15,3,9,11,15,3,9,11,15,3,9,11,15};
         for(int j=0;j<16;j++){uint32_t t=A+H(B,C,D)+X[r3[j]]+0x6ED9EBA1;A=D;D=C;C=B;B=lr(t,s3[j]);}
         a+=A;b+=B;c+=C;d+=D;
     }
     auto le=[](uint32_t x){std::ostringstream o;for(int i=0;i<4;i++)o<<std::hex<<std::setw(2)<<std::setfill('0')<<((x>>(i*8))&0xFF);return o.str();};
     return le(a)+le(b)+le(c)+le(d)+" (NTLM/MD4 of UTF-16LE)";
 }
 
 
 static std::string win1252toUTF8(const std::string &s){
     
     static const uint16_t map[]={
         0x20AC,0,0x201A,0x0192,0x201E,0x2026,0x2020,0x2021,
         0x02C6,0x2030,0x0160,0x2039,0x0152,0,0x017D,0,
         0,0x2018,0x2019,0x201C,0x201D,0x2022,0x2013,0x2014,
         0x02DC,0x2122,0x0161,0x203A,0x0153,0,0x017E,0x0178
     };
     std::string r;
     for(unsigned char c:s){
         uint32_t cp=c;
         if(c>=0x80&&c<=0x9F)cp=map[c-0x80];
         if(cp==0){r+='?';continue;}
         if(cp<0x80)r+=(char)cp;
         else if(cp<0x800){r+=(char)(0xC0|(cp>>6));r+=(char)(0x80|(cp&0x3F));}
         else{r+=(char)(0xE0|(cp>>12));r+=(char)(0x80|((cp>>6)&0x3F));r+=(char)(0x80|(cp&0x3F));}
     }
     return r;
 }
 
 
 static std::string parseCSV(const std::string &s){
     std::istringstream iss(s); std::string line,r; int row=0;
     while(std::getline(iss,line)&&row<20){
         r+="Row "+std::to_string(row+1)+": ";
         std::istringstream ls(line); std::string cell; int col=0;
         while(std::getline(ls,cell,',')){if(col)r+=" | ";r+='"'+trimStr(cell)+'"';col++;}
         r+='\n'; row++;
     }
     return r.empty()?"(empty)":r;
 }
 
 
 static std::string sqlFormat(const std::string &s){
     
     static const std::vector<std::string> kw={"SELECT","FROM","WHERE","JOIN","LEFT","INNER","RIGHT",
         "ORDER BY","GROUP BY","HAVING","LIMIT","INSERT","UPDATE","DELETE","CREATE","DROP","ALTER"};
     std::string r=s;
     for(auto&k:kw){
         size_t pos=0;
         std::string ku=k;
         while((pos=r.find(ku,pos))!=std::string::npos){
             if(pos>0&&r[pos-1]!=' '&&r[pos-1]!='\n')r.insert(pos,"\n");
             pos+=ku.size()+1;
         }
     }
     return r;
 }
 
 
 static std::string hexToCSS(const std::string &s){
     std::string h;for(char c:s)if(isxdigit(c))h+=toupper(c);
     if(h.size()<6)return "(need at least 6 hex digits for RGB)";
     int r2=std::stoi(h.substr(0,2),nullptr,16);
     int g2=std::stoi(h.substr(2,2),nullptr,16);
     int b2=std::stoi(h.substr(4,2),nullptr,16);
     std::ostringstream o;
     o<<"#"<<h.substr(0,6)<<"\n"
      <<"rgb("<<r2<<","<<g2<<","<<b2<<")\n"
      <<"rgba("<<r2<<","<<g2<<","<<b2<<",1.0)\n"
      <<"hsl: approx";
     
     double rf=r2/255.0,gf=g2/255.0,bf2=b2/255.0;
     double mx=std::max({rf,gf,bf2}),mn=std::min({rf,gf,bf2});
     double l=(mx+mn)/2,s2=0,hh=0;
     if(mx!=mn){
         s2=(l>0.5)?(mx-mn)/(2-mx-mn):(mx-mn)/(mx+mn);
         if(mx==rf)hh=fmod((gf-bf2)/(mx-mn),6.0);
         else if(mx==gf)hh=(bf2-rf)/(mx-mn)+2;
         else hh=(rf-gf)/(mx-mn)+4;
         hh*=60;if(hh<0)hh+=360;
     }
     o<<std::fixed<<std::setprecision(1)<<"("<<hh<<"°, "<<s2*100<<"%, "<<l*100<<"%)";
     return o.str();
 }
 
 
 static std::string scanHexForStrings(const std::string &s){
     
     std::string r; std::string run;
     for(unsigned char c:s){
         if(c>=32&&c<127){run+=c;}
         else{if(run.size()>=4){if(!r.empty())r+='\n';r+=run;}run="";}
     }
     if(run.size()>=4){if(!r.empty())r+='\n';r+=run;}
     return r.empty()?"(no printable strings found)":r;
 }
 
 
 static std::string takeFirstBytes(const std::string &s){
     std::string r=s.substr(0,std::min((size_t)64,s.size()));
     return toHexStr(r," ",true)+" [first "+std::to_string(r.size())+" bytes]";
 }
 
 
 static std::string dropFirstByte(const std::string &s){
     if(s.empty())return "(empty)";
     return toHexStr(s.substr(1)," ",true)+" [dropped 1 byte]";
 }
 
 
 static std::string reverseBytes(const std::string &s){
     std::string r(s.rbegin(),s.rend());
     return toHexStr(r," ",true);
 }
 
 
 static std::string detectEncoding(const std::string &s){
     std::string r="Detection hints:\n";
     
     bool b64ok=true;for(char c:s)if(!isalnum(c)&&c!='+'&&c!='/'&&c!='='&&!isspace(c)){b64ok=false;break;}
     if(b64ok&&s.size()%4==0)r+="✓ Looks like Base64\n";
     
     bool hexok=true;std::string nh;for(char c:s)if(!isspace(c))nh+=c;
     for(char c:nh)if(!isxdigit(c)){hexok=false;break;}
     if(hexok&&nh.size()%2==0)r+="✓ Looks like Hex\n";
     
     bool binok=true;for(char c:s)if(c!='0'&&c!='1'&&c!=' '){binok=false;break;}
     if(binok)r+="✓ Looks like Binary\n";
     
     if(s.find('%')!=std::string::npos)r+="✓ May be URL-encoded\n";
     
     if(s.find('&')!=std::string::npos&&s.find(';')!=std::string::npos)r+="✓ May contain HTML entities\n";
     
     int dots=0;for(char c:s)if(c=='.')dots++;
     if(dots==2)r+="✓ Looks like JWT (3 dot-separated parts)\n";
     
     if(s.find("\\u")!=std::string::npos)r+="✓ Contains \\uXXXX unicode escapes\n";
     
     bool morse=true;for(char c:s)if(c!='.'&&c!='-'&&c!=' '&&c!='/'){morse=false;break;}
     if(morse&&!s.empty())r+="✓ Looks like Morse code\n";
     
     r+="\nEntropy: "+entropy(s);
     return r;
 }
 
 
 static std::string zlibHeaderCheck(const std::string &s){
     if(s.size()<2)return "too short";
     unsigned char b0=(unsigned char)s[0], b1=(unsigned char)s[1];
     std::string r;
     r+="Byte 0: 0x"+toHexStr(s.substr(0,1),"",true)+" Byte 1: 0x"+toHexStr(s.substr(1,1),"",true)+"\n";
     if(b0==0x78){
         r+="zlib magic (0x78) detected\n";
         if(b1==0x9C)r+="Default compression (zlib)";
         else if(b1==0xDA)r+="Best compression";
         else if(b1==0x01)r+="No/low compression";
         else if(b1==0x5E)r+="Fast compression";
     } else if(b0==0x1F&&b1==0x8B) r+="gzip magic detected (1F 8B)";
     else if(b0==0x42&&b1==0x5A) r+="bzip2 magic (BZ)";
     else if(b0==0x50&&b1==0x4B) r+="ZIP/PK magic";
     else if(b0==0x7F&&s.size()>3&&s[1]=='E'&&s[2]=='L'&&s[3]=='F') r+="ELF binary";
     else if(b0==0xFF&&b1==0xD8) r+="JPEG image";
     else if(b0==0x89&&b1==0x50) r+="PNG image";
     else r+="unknown magic bytes";
     return r;
 }
 
 
 static std::string fromBase(const std::string &s, int base){
     std::istringstream iss(s); std::string tok;
     std::string r;
     while(iss>>tok){
         try{
             unsigned long long v=std::stoull(tok,nullptr,base);
             if(!r.empty())r+=' ';
             if(v>=32&&v<127)r+=(char)v;
             else{std::ostringstream o;o<<"[0x"<<std::hex<<v<<"]";r+=o.str();}
         }catch(...){r+="?";}
     }
     return r.empty()?"(no valid tokens)":r;
 }
 
 
 static std::string simpleKDF(const std::string &s){
     
     std::string h=s;
     for(int i=0;i<1024;i++){h=sha256(h);}
     return "KDF-1024×SHA256: "+h+"\n(use bcrypt/argon2 in production)";
 }
 
 
 
 
 
 struct Entry {
     std::string name;
     Fn fn;
 };
 
 struct Category {
     std::string name;
     std::vector<Entry> entries;
 };
 
 static std::vector<Category> CATS = {
 {"NUMERIC BASES", {
     {"Binary (8-bit spaced)",   binarySpaced},
     {"Binary (stream)",         binaryStream},
     {"Octal (3-digit spaced)",  toOctal},
     {"Decimal codepoints",      toDecimal},
     {"Hex UPPERCASE",           [](auto&s){return toHexStr(s," ",true);}},
     {"Hex lowercase",           hexLower},
     {"Hex 0x-prefixed",         toHex0x},
     {"Hex compact (no spaces)", hexCompact},
     {"Hex colon-separated",     hexColon},
     {"Hex C array",             hexCArray},
     {"Hex dump (xxd)",          hexDump},
     {"Base-32",                 base32Enc},
     {"Base-64",                 [](auto&s){return base64Enc(s);}},
     {"Base-64 URL-safe",        [](auto&s){return base64Enc(s,true,true);}},
     {"Base-64 no padding",      [](auto&s){return base64Enc(s,false,false);}},
     {"Octal (\\0-escaped)",     [](auto&s){
         std::string r;
         for(unsigned char c:s){std::ostringstream o;o<<"\\0"<<std::oct<<(int)c;r+=o.str()+" ";}
         return r;
     }},
     {"Multi-base (numeric)",    multiBase},
 }},
 {"ENCODINGS & ESCAPES", {
     {"URL encode",              urlEncode},
     {"URL encode (all chars)",  percentEncodeAll},
     {"HTML entities",           htmlEncode},
     {"XML entities",            xmlEncode},
     {"Unicode codepoints",      unicodeCodepoints},
     {"Unicode \\uXXXX escapes", unicodeEscape},
     {"JSON unicode escapes",    jsonUnicode},
     {"C/C++ string literal",    cStringLiteral},
     {"Python bytes literal",    pythonBytes},
     {"Quoted-Printable (MIME)", quotedPrintable},
     {"UUencoding (line 1)",     uuencode},
     {"UTF-16 BE hex",           utf16BE},
     {"UTF-32 BE hex",           utf32BE},
     {"Null-byte padded hex",    nullPadded},
     {"ASCII compat (punycode)", asciiCompat},
 }},
 {"CLASSICAL CIPHERS", {
     {"ROT-1",                   rot1},
     {"ROT-5 (digits only)",     [](auto&s){std::string r;for(char c:s){if(c>='0'&&c<='9')r+=(char)('0'+(c-'0'+5)%10);else r+=c;}return r;}},
     {"ROT-13",                  rot13},
     {"ROT-18 (alphanum)",       rot18},
     {"ROT-47 (printable)",      rot47},
     {"Atbash",                  atbash},
     {"Caesar +3",               [](auto&s){return rotN(s,3);}},
     {"Caesar +7",               [](auto&s){return rotN(s,7);}},
     {"Caesar +15",              [](auto&s){return rotN(s,15);}},
     {"Vigenere (key=KEY)",      [](auto&s){return vigenere(s,"KEY");}},
     {"Vigenere (key=SECRET)",   [](auto&s){return vigenere(s,"SECRET");}},
     {"Affine (a=3,b=7)",        [](auto&s){return affine(s,3,7);}},
     {"Affine (a=5,b=8)",        [](auto&s){return affine(s,5,8);}},
     {"Bacon's cipher (A/B)",    baconAB},
     {"Polybius square",         polybius},
     {"Tap code",                tapCode},
     {"Rail fence (3 rails)",    [](auto&s){return railFence(s,3);}},
     {"Rail fence (4 rails)",    [](auto&s){return railFence(s,4);}},
 }},
 {"TEXT TRANSFORMATIONS", {
     {"UPPERCASE",               toUpper},
     {"lowercase",               toLower},
     {"Title Case",              toTitle},
     {"Alternating cAsE",        alternatingCase},
     {"camelCase",               toCamelCase},
     {"snake_case",              toSnakeCase},
     {"kebab-case",              toKebabCase},
     {"SCREAMING_SNAKE_CASE",    toScreamingSnake},
     {"Reverse string",          reverseStr},
     {"Reverse words",           reverseWords},
     {"Reverse each word",       reverseEachWord},
     {"Pig Latin",               pigLatin},
     {"Leet speak (1337)",       leet},
     {"NATO phonetic",           natoPhonetic},
     {"Phonetic digits",         phoneticDigits},
     {"Run-length encoding",     runLength},
     {"Run-length (binary RLE)", runLengthBin},
     {"8-char chunks",           [](auto&s){return chunks(s,8);}},
     {"4-char chunks",           [](auto&s){return chunks(s,4);}},
     {"ASCII block art",         asciiBlock},
 }},
 {"UNICODE STYLES", {
     {"𝐌𝐚𝐭𝐡 𝐁𝐨𝐥𝐝",             mathBold},
     {"𝑀𝑎𝑡ℎ 𝐼𝑡𝑎𝑙𝑖𝑐",            mathItalic},
     {"𝔻𝕠𝕦𝕓𝕝𝕖 𝕊𝕥𝕣𝕦𝕔𝕜",         mathDouble},
     {"ＦＵＬＬＷＩＤＴＨ",            fullwidth},
     {"Sᴍᴀʟʟ Cᴀᴘs",             smallCaps},
     {"Sᵘᵖᵉʳˢᶜʳⁱᵖᵗ",             superscript},
     {"Sᵤᵦₛcᵣᵢₚₜ",               subscript},
     {"⓪①② Bubble text",        bubbleText},
     {"⠓⠑⠇⠇⠕ Braille",          brailleEncode},
     {"ɥoʇʇoɯ-dn uʍop",         upsideDown},
     {"Z̶a̷l̵g̷o̸",                  zalgo},
 }},
 {"COMMUNICATION CODES", {
     {"Morse code",              morseEncode},
     {"Semaphore positions",     [](auto&s){
         static std::map<char,std::string> m={
             {'A',"↑→"},{'B',"↑↗"},{'C',"↑↓"},{'D',"↑↙"},{'E',"↑←"},
             {'F',"↑↖"},{'G',"↑↘"},{'H',"→↗"},{'I',"→↓"},{'J',"←↗"},
             {'K',"↖↓"},{'L',"↙↓"},{'M',"↗↓"},{'N',"↗↙"},{'O',"↗←"},
             {'P',"↗↖"},{'Q',"↗↘"},{'R',"↓↙"},{'S',"↓←"},{'T',"↓↖"},
             {'U',"↙←"},{'V',"↘←"},{'W',"←↖"},{'X',"←↘"},{'Y',"↖↘"},
             {'Z',"↓↘"},{' ',"   "}
         };
         std::string r;
         for(char c:s){char u=toupper(c);if(m.count(u)){if(!r.empty())r+=' ';r+=m[u];}else if(c==' ')r+="  ";}
         return r;
     }},
     {"Phonetic NATO",           natoPhonetic},
     {"Soundex",                 soundex},
     {"Numeric (dec per char)",  numericWords},
 }},
 {"XOR / BITWISE", {
     {"XOR 0xFF (flip all bits)",xorFF},
     {"XOR 0xAA",                xorAA},
     {"XOR 0xDEADBEEF",          xorDeadBeef},
     {"XOR 0x42",                [](auto&s){return xorKey(s,0x42);}},
     {"XOR 0x55",                [](auto&s){return xorKey(s,0x55);}},
     {"Bit count analysis",      bitCount},
     {"Byte sum",                byteSum},
     {"XOR checksum",            xorChecksum},
     {"Big-endian int (8 bytes)",toIntBigEndian},
 }},
 {"HASHES / CHECKSUMS", {
     {"DJB2 hash",               djb2Hash},
     {"FNV-1a (32-bit)",         fnv1aHash},
     {"Adler-32",                adler32Hash},
     {"CRC-8",                   crc8Hash},
     {"CRC-16/CCITT",            crc16Hash},
     {"Sum of codepoints",       sumCodepoints},
     {"Product (mod 10^9+7)",    productCodepoints},
     {"Entropy (bits/symbol)",   entropy},
 }},
 {"ANALYSIS & STATS", {
     {"Character stats",         charStats},
     {"Char frequency (top 10)", freqAnalysis},
     {"Word frequency (top 8)",  wordFreq},
     {"String info",             stringInfo},
     {"Bit count",               bitCount},
     {"Temperature convert",     tempConvert},
     {"Numeric multi-base",      multiBase},
     {"Roman numerals",          toRoman},
 }},
 
 {"FROM / DECODE", {
     {"From Base64",             base64Dec},
     {"From Base32",             base32Dec},
     {"From Hex",                fromHex},
     {"From Hex dump",           fromHexDump},
     {"From Binary",             fromBinary},
     {"From Octal",              fromOctal},
     {"From Decimal",            fromDecimal},
     {"From Charcode (dec)",     fromCharcode},
     {"From Base (auto)",        [](auto&s){return fromBase(s,16);}},
     {"HTML entity decode",      htmlDecode},
     {"URL decode",              urlDecode},
     {"Unescape string",         unescapeString},
     {"From UNIX timestamp",     fromUnixTimestamp},
     {"Hex → decimal",           hexToDec},
     {"Dec → hex",               decToHex},
     {"Hex → float (32-bit)",    hexToFloat},
     {"JWT decode",              jwtDecode},
     {"Google ei timestamp",     googleEiTimestamp},
     {"Windows FILETIME",        filetimeToUnix},
     {"From Base (base-16 str)", [](auto&s){return fromBase(s,16);}},
 }},
 {"TO / ENCODE", {
     {"To Charcode (decimal)",   toCharcode},
     {"To Charcode (hex)",       toCharcodeHex},
     {"Escape string",           escapeString},
     {"HTML entities (full)",    escapeHTMLFull},
     {"Defang URL",              defangURL},
     {"Refang URL",              refangURL},
     {"Strip HTML tags",         stripHTML},
     {"JSON prettify",           jsonPrettify},
     {"JSON minify",             jsonMinify},
     {"To UNIX timestamp",       toUnixTimestamp},
     {"Win-1252 → UTF-8",        win1252toUTF8},
     {"Swap endianness (32-bit)",swapEndian32},
     {"Format number",           formatNumber},
     {"Hex → CSS color",         hexToCSS},
 }},
 {"NETWORK & PARSING", {
     {"Parse IPv4",              parseIPv4},
     {"Parse IPv6",              parseIPv6},
     {"Parse CIDR",              parseCIDR},
     {"Extract URLs",            extractURLs},
     {"Extract IPs",             extractIPs},
     {"Extract emails",          extractEmails},
     {"Parse UUID / GUID",       parseUUID},
     {"Parse date/time",         parseDateTime},
     {"Parse CSV (preview)",     parseCSV},
     {"SQL format",              sqlFormat},
     {"Zlib/magic bytes check",  zlibHeaderCheck},
     {"Scan for strings",        scanHexForStrings},
     {"Detect encoding",         detectEncoding},
     {"Protobuf decode",         decodeProtobuf},
 }},
 {"HASHING (FULL)", {
     {"MD5",                     md5},
     {"SHA-1",                   sha1},
     {"SHA-256",                 sha256},
     {"HMAC-SHA256 (key space msg)",hmacSha256},
     {"NTLM (MD4/UTF-16LE)",     ntlmHash},
     {"DJB2",                    djb2Hash},
     {"FNV-1a 32-bit",           fnv1aHash},
     {"Adler-32",                adler32Hash},
     {"CRC-8",                   crc8Hash},
     {"CRC-16/CCITT",            crc16Hash},
     {"KDF 1024×SHA256",         simpleKDF},
 }},
 {"CRYPTO & CIPHERS +", {
     {"Enigma (M3, I-II-III)",   enigmaSimple},
     {"Beaufort cipher",         beaufort},
     {"Half-way substitute",     halfwaySubstitute},
     {"ROT13 + digits (ROT18)",  rot13WithDigits},
     {"Caesar brute-force",      caesarBruteForce},
     {"XOR single-byte bruteforce",xorBruteForce},
     {"Stream cipher demo",      rabbitCipher},
 }},
 {"MATH & ARITHMETIC", {
     {"Add (a b)",               add},
     {"Subtract (a b)",          subtract},
     {"Multiply (a b)",          multiply},
     {"Divide (a b)",            divideOp},
     {"Modulo (a b)",            moduloOp},
     {"Power (a b)",             powerOp},
     {"Square root",             sqrtOp},
     {"Logarithm (ln/log2/log10)",logOp},
     {"Bitwise AND (a b)",       bitwiseAND},
     {"Bitwise OR (a b)",        bitwiseOR},
     {"Bitwise NOT",             bitwiseNOT},
     {"Shift left (a n)",        shiftLeft},
     {"Shift right (a n)",       shiftRight},
     {"Convert data units",      convertDataUnits},
     {"Temperature convert",     tempConvert},
 }},
 {"TEXT & DATA OPS", {
     {"Remove whitespace",       removeWhitespace},
     {"Remove line breaks",      removeLinebreaks},
     {"Remove null bytes",       removeNullBytes},
     {"Trim",                    trimStr},
     {"Head (first 10 lines)",   headLines},
     {"Tail (last 10 lines)",    tailLines},
     {"Sort lines",              sortLines},
     {"Unique lines",            uniqueLines},
     {"Count lines/bytes",       countLines},
     {"Pad start (32 chars)",    padStart},
     {"Pad end (32 chars)",      padEnd},
     {"CRLF → LF",               [](auto&s){std::string r;for(size_t i=0;i<s.size();i++){if(s[i]=='\r'&&i+1<s.size()&&s[i+1]=='\n'){r+='\n';i++;}else r+=s[i];}return r;}},
     {"LF → CRLF",               [](auto&s){std::string r;for(char c:s){if(c=='\n')r+='\r';r+=c;}return r;}},
     {"Hex to newlined hex",     toHexNL},
     {"Take first 64 bytes",     takeFirstBytes},
     {"Drop first byte",         dropFirstByte},
     {"Reverse bytes (hex)",     reverseBytes},
 }},
 {"FREQUENCY ANALYSIS", {
     {"Index of coincidence",    indexOfCoincidence},
     {"Chi-squared (vs English)",chiSquared},
     {"Char frequency",          freqAnalysis},
     {"Entropy",                 entropy},
     {"Caesar brute-force",      caesarBruteForce},
     {"XOR brute-force (1-byte)",xorBruteForce},
 }},
 };
 
 
 struct FlatEntry { std::string cat; std::string name; Fn fn; };
 static std::vector<FlatEntry> FLAT;
 
 void buildFlat(){
     FLAT.clear();
     for(auto&cat:CATS)
         for(auto&e:cat.entries)
             FLAT.push_back({cat.name,e.name,e.fn});
 }
 
 } 
 
 
 
 
 
 struct App {
     
     std::string input;
     int inputCursor = 0;
     std::string filterText;
     bool filterMode = false;
 
     int selectedFlat = 0;   
     int navScroll = 0;
 
     struct Output { std::string label; std::string value; bool ok; };
     std::vector<Output> outputs;
     int outScroll = 0;
 
     enum Pane { PANE_NAV, PANE_INPUT, PANE_OUTPUT } activePane = PANE_INPUT;
 
     bool running = true;
     std::string statusMsg;
     int statusTimer = 0;
 
     
     std::vector<int> filtered; 
 
     void buildFiltered(){
         filtered.clear();
         std::string fl = filterText;
         for(auto&c:fl)c=tolower(c);
         for(int i=0;i<(int)Enc::FLAT.size();i++){
             if(fl.empty()){filtered.push_back(i);continue;}
             std::string n=Enc::FLAT[i].name+Enc::FLAT[i].cat;
             for(auto&c:n)c=tolower(c);
             if(n.find(fl)!=std::string::npos)filtered.push_back(i);
         }
         if(selectedFlat>=(int)filtered.size())selectedFlat=std::max(0,(int)filtered.size()-1);
     }
 
     void runEncoding(int fi){
         if(fi<0||fi>=(int)filtered.size())return;
         int idx=filtered[fi];
         auto&e=Enc::FLAT[idx];
         outputs.clear();
         std::string val;
         try{ val=e.fn(input); outputs.push_back({e.name,val,true}); }
         catch(std::exception&ex){ outputs.push_back({e.name,std::string("ERROR: ")+ex.what(),false}); }
         outScroll=0;
         setStatus("Encoded: "+e.name);
     }
 
     void runAll(){
         outputs.clear();
         for(int fi:filtered){
             auto&e=Enc::FLAT[fi];
             try{ outputs.push_back({e.name,e.fn(input),true}); }
             catch(std::exception&ex){ outputs.push_back({e.name,"ERR: "+std::string(ex.what()),false}); }
         }
         outScroll=0;
         setStatus("ENCODE ALL: "+std::to_string(outputs.size())+" encodings run");
     }
 
     void setStatus(const std::string &m){ statusMsg=m; statusTimer=40; }
 
     
 
     void draw(){
         Term::getSize();
         int W=Term::COLS, H=Term::ROWS;
         std::string buf;
         buf.reserve(1<<17);
 
         
         buf+=Term::hideCur()+Term::mv(1,1);
 
         
         buf+=Term::bg(Term::C_BG)+Term::fg(Term::C_GREEN)+Term::bold();
         auto title=std::string(" ◈ OMNI-TRANSLATE  //  UNIVERSAL ENCODER  //  EVERY. ENCODING. POSSIBLE. ");
         while((int)title.size()<W)title+=' ';
         buf+=title.substr(0,W);
         buf+=Term::reset();
 
         
         buf+=Term::mv(2,1)+Term::bg(Term::C_PANEL_BG)+Term::fg(Term::C_AMBER)+Term::dim();
         auto sub=std::string(" [TAB] switch pane  [↑↓] navigate  [ENTER] encode  [Ctrl+A] encode all  [/] search  [Ctrl+X] swap I/O  [q] quit");
         while((int)sub.size()<W)sub+=' ';
         buf+=sub.substr(0,W)+Term::reset();
 
         
         int navW=32;
         int topH=9;  
         int startRow=3;
 
         
         int navRows=H-4; 
         drawPanel(buf,1,startRow,navW,navRows,
                   " ENCODINGS ["+std::to_string(filtered.size())+"] ",
                   activePane==PANE_NAV);
         drawNavContent(buf,2,startRow+1,navW-2,navRows-2);
 
         
         int inputCol=navW+1;
         int inputW=W-navW;
         drawPanel(buf,inputCol,startRow,inputW,topH,
                   " INPUT BUFFER ",activePane==PANE_INPUT);
         drawInputContent(buf,inputCol+1,startRow+1,inputW-2,topH-2);
 
         
         int outRow=startRow+topH;
         int outH=H-outRow-1;
         if(outH<3)outH=3;
         drawPanel(buf,inputCol,outRow,inputW,outH,
                   " OUTPUT ["+ std::to_string(outputs.size()) +" results] ",
                   activePane==PANE_OUTPUT);
         drawOutputContent(buf,inputCol+1,outRow+1,inputW-2,outH-2);
 
         
         buf+=Term::mv(H,1)+Term::bg(Term::C_PANEL_BG)+Term::fg(Term::C_GREEN_DIM);
         std::string st = statusTimer>0 ? " ◈ "+statusMsg : " ◈ READY │ chars="+std::to_string(input.size())+" │ mode="+(filterMode?"FILTER":"NORMAL")+" │ pane="+(activePane==PANE_NAV?"NAV":activePane==PANE_INPUT?"INPUT":"OUTPUT");
         while((int)st.size()<W)st+=' ';
         buf+=st.substr(0,W)+Term::reset();
 
         
         if(activePane==PANE_INPUT){
             
             int cx=inputCol+1;
             int cy=startRow+1;
             
             int col=cx,row=cy;
             std::string vis=input;
             int iw=inputW-2;
             for(int i=0;i<inputCursor&&i<(int)vis.size();i++){
                 if(vis[i]=='\n'||col-cx>=iw-1){col=cx;row++;}
                 else col++;
             }
             if(row<startRow+topH-1){
                 buf+=Term::mv(row,col)+Term::showCur();
             }
         }
 
         Term::write_str(buf);
         if(statusTimer>0)statusTimer--;
     }
 
     void drawPanel(std::string &buf,int x,int y,int w,int h,
                    const std::string &title,bool active){
         using namespace Term;
         int tc=active?C_GREEN:C_BORDER;
         
         buf+=mv(y,x)+fg(tc);
         buf+="┌";
         std::string t=" "+title+" ";
         int remain=w-2-(int)t.size();
         if(remain<0)remain=0;
         buf+=t;
         for(int i=0;i<remain;i++)buf+="─";
         buf+="┐"+reset();
         
         for(int r=1;r<h-1;r++){
             buf+=mv(y+r,x)+fg(tc)+"│"+reset();
             
             buf+=bg(C_BG);
             for(int c=1;c<w-1;c++)buf+=' ';
             buf+=reset()+fg(tc)+"│"+reset();
         }
         
         buf+=mv(y+h-1,x)+fg(tc)+"└";
         for(int i=0;i<w-2;i++)buf+="─";
         buf+="┘"+reset();
     }
 
     void drawNavContent(std::string &buf,int x,int y,int w,int h){
         using namespace Term;
         
         buf+=mv(y,x);
         if(filterMode){
             buf+=fg(C_AMBER)+bold()+"/"+ reset()+fg(C_WHITE)+filterText;
             std::string pad(std::max(0,w-1-(int)filterText.size()),' ');
             buf+=pad+reset();
         } else {
             buf+=fg(C_GREEN_DIM)+dim()+"/ search encodings";
             std::string pad(std::max(0,w-18),' ');
             buf+=pad+reset();
         }
 
         int listH=h-1;
         
         if(selectedFlat-navScroll>=listH) navScroll=selectedFlat-listH+1;
         if(selectedFlat<navScroll) navScroll=selectedFlat;
         if(navScroll<0)navScroll=0;
 
         for(int i=0;i<listH;i++){
             int fi=navScroll+i;
             buf+=mv(y+1+i,x);
             if(fi>=(int)filtered.size()){
                 
                 buf+=bg(C_BG)+std::string(w,' ')+reset();
                 continue;
             }
             int idx=filtered[fi];
             bool sel=(fi==selectedFlat);
             if(sel){
                 buf+=bg(C_SEL_BG)+fg(C_SEL_FG)+bold();
             } else {
                 buf+=bg(C_BG)+fg(C_GREEN_DIM);
             }
             std::string name=Enc::FLAT[idx].name;
             if((int)name.size()>w-2)name=name.substr(0,w-3)+"…";
             std::string line=" "+name;
             while((int)line.size()<w)line+=' ';
             buf+=line.substr(0,w)+reset();
         }
     }
 
     void drawInputContent(std::string &buf,int x,int y,int w,int h){
         using namespace Term;
         
         std::vector<std::string> lines;
         std::string cur;
         for(char c:input){
             if(c=='\n'||((int)cur.size()>=w-1)){lines.push_back(cur);cur="";}
             if(c!='\n')cur+=c;
         }
         lines.push_back(cur);
 
         int textH=h-1; 
         for(int i=0;i<textH;i++){
             buf+=mv(y+i,x)+bg(C_BG);
             if(i<(int)lines.size()){
                 buf+=fg(C_GREEN);
                 std::string l=lines[i];
                 if((int)l.size()<w)l+=std::string(w-l.size(),' ');
                 buf+=l.substr(0,w)+reset();
             } else {
                 buf+=std::string(w,' ')+reset();
             }
         }
         
         buf+=mv(y+h-1,x)+bg(C_PANEL_BG)+fg(C_AMBER)+dim();
         std::string stats=" chars="+std::to_string(input.size());
         int words=0;bool inw=false;for(char c:input){if(!isspace(c)){if(!inw){words++;inw=true;}}else inw=false;}
         stats+=" words="+std::to_string(words);
         int lines2=1;for(char c:input)if(c=='\n')lines2++;
         stats+=" lines="+std::to_string(lines2);
         while((int)stats.size()<w)stats+=' ';
         buf+=stats.substr(0,w)+reset();
     }
 
     void drawOutputContent(std::string &buf,int x,int y,int w,int h){
         using namespace Term;
         if(outputs.empty()){
             buf+=mv(y+h/2,x)+fg(C_GREEN_DIM)+dim();
             std::string msg="  ░░ SELECT ENCODING OR HIT Ctrl+A FOR ALL ░░  ";
             if((int)msg.size()<w)msg+=std::string(w-msg.size(),' ');
             buf+=msg.substr(0,w)+reset();
             return;
         }
 
         
         struct Line { std::string text; int color; bool bold_; };
         std::vector<Line> lines;
 
         for(auto&out:outputs){
             
             std::string lbl="──[ "+out.label+" ]";
             while((int)lbl.size()<w-1)lbl+="─";
             lines.push_back({lbl,C_LABEL,false});
             
             std::string val=out.value;
             if(val.empty())val="(empty)";
             int vc=out.ok?C_OUTPUT:C_RED;
             
             for(size_t i=0;i<val.size();){
                 
                 size_t nl=val.find('\n',i);
                 size_t end=(nl==std::string::npos)?val.size():nl;
                 std::string seg=val.substr(i,end-i);
                 while((int)seg.size()>w-2){
                     lines.push_back({"  "+seg.substr(0,w-2),vc,false});
                     seg=seg.substr(w-2);
                 }
                 lines.push_back({"  "+seg,vc,false});
                 i=end+(nl==std::string::npos?0:1);
             }
             lines.push_back({"",C_GREEN_DIM,false});
         }
 
         int total=(int)lines.size();
         if(outScroll>total-h)outScroll=std::max(0,total-h);
         if(outScroll<0)outScroll=0;
 
         for(int i=0;i<h;i++){
             int li=outScroll+i;
             buf+=mv(y+i,x)+bg(C_BG);
             if(li<total){
                 buf+=fg(lines[li].color);
                 if(lines[li].bold_)buf+=bold();
                 std::string l=lines[li].text;
                 
                 while((int)l.size()<w)l+=' ';
                 buf+=l.substr(0,w)+reset();
             } else {
                 buf+=std::string(w,' ')+reset();
             }
         }
 
         
         if(total>h){
             buf+=mv(y,x+w-6)+fg(C_AMBER)+dim();
             std::string si="↑"+std::to_string(outScroll)+"/"+std::to_string(total);
             buf+=si+reset();
         }
     }
 
     
 
     void handleKey(Term::Key k){
         if(filterMode){
             handleFilterKey(k); return;
         }
         switch(activePane){
             case PANE_INPUT:  handleInputPaneKey(k); break;
             case PANE_NAV:    handleNavKey(k); break;
             case PANE_OUTPUT: handleOutputKey(k); break;
         }
         
         switch(k.type){
             case Term::Key::MOUSE_PRESS:
             case Term::Key::MOUSE_RELEASE:
             case Term::Key::MOUSE_SCROLL_UP:
             case Term::Key::MOUSE_SCROLL_DOWN:
                 handleMouse(k); return;
             case Term::Key::TAB:
                 if(activePane==PANE_INPUT) activePane=PANE_NAV;
                 else if(activePane==PANE_NAV) activePane=PANE_OUTPUT;
                 else activePane=PANE_INPUT;
                 break;
             case Term::Key::CTRL_A: runAll(); activePane=PANE_OUTPUT; break;
             case Term::Key::CTRL_L: input.clear(); inputCursor=0; outputs.clear(); break;
             case Term::Key::CTRL_X: swapIO(); break;
             case Term::Key::CHAR:
                 if(k.ch=='q'&&activePane!=PANE_INPUT){running=false;}
                 if(k.ch=='/'){filterMode=true;filterText="";}
                 break;
         }
     }
 
     void handleInputPaneKey(Term::Key k){
         switch(k.type){
             case Term::Key::CHAR:
                 input.insert(inputCursor,1,k.ch);
                 inputCursor++;
                 break;
             case Term::Key::ENTER:
                 input.insert(inputCursor,1,'\n');
                 inputCursor++;
                 break;
             case Term::Key::BACKSPACE:
                 if(inputCursor>0){input.erase(inputCursor-1,1);inputCursor--;}
                 break;
             case Term::Key::DEL:
                 if(inputCursor<(int)input.size())input.erase(inputCursor,1);
                 break;
             case Term::Key::LEFT:
                 if(inputCursor>0)inputCursor--;
                 break;
             case Term::Key::RIGHT:
                 if(inputCursor<(int)input.size())inputCursor++;
                 break;
             case Term::Key::HOME: inputCursor=0; break;
             case Term::Key::END:  inputCursor=(int)input.size(); break;
             default: break;
         }
     }
 
     void handleNavKey(Term::Key k){
         switch(k.type){
             case Term::Key::UP:
                 if(selectedFlat>0)selectedFlat--;
                 break;
             case Term::Key::DOWN:
                 if(selectedFlat<(int)filtered.size()-1)selectedFlat++;
                 break;
             case Term::Key::PGUP: selectedFlat=std::max(0,selectedFlat-10); break;
             case Term::Key::PGDN: selectedFlat=std::min((int)filtered.size()-1,selectedFlat+10); break;
             case Term::Key::HOME: selectedFlat=0; break;
             case Term::Key::END:  selectedFlat=(int)filtered.size()-1; break;
             case Term::Key::ENTER:
                 if(!input.empty())runEncoding(selectedFlat);
                 else setStatus("⚠ Input buffer is empty");
                 activePane=PANE_OUTPUT;
                 break;
             default: break;
         }
     }
 
     void handleOutputKey(Term::Key k){
         switch(k.type){
             case Term::Key::UP:   if(outScroll>0)outScroll--; break;
             case Term::Key::DOWN: outScroll++; break;
             case Term::Key::PGUP: outScroll=std::max(0,outScroll-20); break;
             case Term::Key::PGDN: outScroll+=20; break;
             case Term::Key::HOME: outScroll=0; break;
             default: break;
         }
     }
 
     void handleFilterKey(Term::Key k){
         switch(k.type){
             case Term::Key::ESCAPE: filterMode=false; filterText=""; buildFiltered(); break;
             case Term::Key::ENTER:  filterMode=false; buildFiltered(); activePane=PANE_NAV; break;
             case Term::Key::BACKSPACE:
                 if(!filterText.empty())filterText.pop_back();
                 buildFiltered();
                 break;
             case Term::Key::CHAR:
                 filterText+=k.ch;
                 buildFiltered();
                 break;
             default: break;
         }
     }
 
     
     
     int _navW()    { return 32; }
     int _topH()    { return 9; }
     int _startRow(){ return 3; }
     int _inputCol(){ return _navW()+1; }
     int _outRow()  { return _startRow()+_topH(); }
 
     void handleMouse(Term::Key k){
         int mx=k.mx, my=k.my;
         int W=Term::COLS, H=Term::ROWS;
         int navW=_navW(), startRow=_startRow();
         int inputCol=_inputCol();
         int outRow=_outRow();
         int outH=H-outRow-1; if(outH<3)outH=3;
         (void)_topH();
 
         
         if(k.type==Term::Key::MOUSE_SCROLL_UP){
             if(mx<=navW){ if(selectedFlat>0)selectedFlat--; }
             else if(my>=outRow){ if(outScroll>0)outScroll--; }
             return;
         }
         if(k.type==Term::Key::MOUSE_SCROLL_DOWN){
             if(mx<=navW){ if(selectedFlat<(int)filtered.size()-1)selectedFlat++; }
             else if(my>=outRow){ outScroll++; }
             return;
         }
         if(k.type!=Term::Key::MOUSE_PRESS) return;
 
         
         if(mx>=1 && mx<=navW){
             activePane=PANE_NAV;
             int navContentY=startRow+2; 
             int listRow=my-navContentY;
             
             
             if(my==startRow+2){
                 filterMode=true;
                 return;
             }
             if(listRow>=0){
                 int fi=navScroll+listRow;
                 if(fi>=0&&fi<(int)filtered.size()){
                     selectedFlat=fi;
                     
                     
                 }
             }
             return;
         }
 
         
         if(mx>navW && my>=startRow && my<outRow){
             activePane=PANE_INPUT;
             
             int cx=inputCol+1;
             int cy=startRow+1;
             int iw=Term::COLS-navW-2;
             int clickLine=my-cy;
             int clickCol=mx-cx;
             if(clickLine<0)clickLine=0;
             if(clickCol<0)clickCol=0;
             
             int line=0, col=0, idx=0;
             inputCursor=0;
             for(int i=0;i<(int)input.size();i++){
                 if(line==clickLine&&col==clickCol){ inputCursor=i; return; }
                 if(input[i]=='\n'||col>=iw-1){
                     if(line==clickLine){ inputCursor=i; return; }
                     line++; col=0;
                     if(input[i]!='\n') col=1;
                 } else col++;
                 idx=i+1;
             }
             inputCursor=idx; 
             return;
         }
 
         
         if(mx>navW && my>=outRow && my<outRow+outH){
             activePane=PANE_OUTPUT;
             
             
             
             int clickedLine=outScroll+(my-outRow-1);
             
             int lineIdx=0;
             for(auto&out:outputs){
                 lineIdx++; 
                 
                 std::string val=out.value;
                 int linesForVal=1;
                 int lw=Term::COLS-navW-4;
                 for(size_t pos=0;pos<val.size();){
                     size_t nl=val.find('\n',pos);
                     size_t end=(nl==std::string::npos)?val.size():nl;
                     std::string seg=val.substr(pos,end-pos);
                     linesForVal+=(int)seg.size()/std::max(1,lw);
                     pos=end+(nl==std::string::npos?0:1);
                 }
                 if(clickedLine<lineIdx+linesForVal){
                     
                     std::string enc_val;
                     
                     const std::string b64="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
                     int v2=0,vb=-6;
                     for(unsigned char vc:out.value){
                         v2=(v2<<8)+vc; vb+=8;
                         while(vb>=0){enc_val+=b64[(v2>>vb)&0x3F];vb-=6;}
                     }
                     if(vb>-6)enc_val+=b64[((v2<<8)>>(vb+8))&0x3F];
                     while(enc_val.size()%4)enc_val+='=';
                     Term::write_str("\x1b]52;c;"+enc_val+"\x07");
                     setStatus("◈ COPIED: "+out.label);
                     return;
                 }
                 lineIdx+=linesForVal;
                 lineIdx++; 
             }
             return;
         }
 
         (void)W;
     }
 
     void swapIO(){
         if(outputs.size()==1){
             input=outputs[0].value;
             inputCursor=(int)input.size();
             outputs.clear();
             setStatus("Swapped output → input");
         } else if(!outputs.empty()){
             setStatus("Can't swap: multiple outputs (select one first)");
         } else {
             setStatus("Nothing to swap");
         }
     }
 
     void run(){
         Enc::buildFlat();
         buildFiltered();
 
         Term::rawOn();
         Term::mouseOn();
         Term::write_str(Term::clrscr()+Term::hideCur());
 
         while(running){
             draw();
             Term::Key k=Term::readKey();
             if(k.type!=Term::Key::NONE)
                 handleKey(k);
         }
 
         Term::write_str(Term::showCur()+Term::clrscr());
         Term::mouseOff();
         Term::rawOff();
         std::cout<<"OMNI-TRANSLATE — goodbye.\n";
     }
 };
 
 static void onResize(int){ Term::getSize(); }
 
 int main(){
     signal(SIGWINCH, onResize);
     Term::getSize();
     App app;
     app.run();
     return 0;
 }
