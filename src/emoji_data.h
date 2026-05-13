#ifndef EMOJI_DATA_H
#define EMOJI_DATA_H

typedef struct { const char *glyph; const char *keys; } Glyph;

/* ── Emojis ──────────────────────────────────────────────────────────── */
static const Glyph EMOJIS[] = {
    /* Faces */
    {"😀","happy smile grin face"},{"😂","laugh joy tears funny"},{"😍","love heart eyes"},
    {"🥰","love hearts smiling"},{"😘","kiss love blow"},{"😊","smile blush happy"},
    {"😇","innocent angel halo"},{"🙂","slight smile"},{"😉","wink"},{"😋","yum tongue"},
    {"😎","cool sunglasses"},{"🤔","thinking hmm"},{"🤗","hug hugging"},{"😐","neutral"},
    {"🙄","eye roll"},{"😏","smirk"},{"😒","unamused unhappy"},{"😔","pensive sad"},
    {"😢","cry sad tear"},{"😭","sob crying loudly"},{"😱","scream fear horror"},
    {"😡","angry red mad"},{"😠","angry"},{"🤬","swearing cursing"},
    {"😈","devil evil"},{"👿","devil angry"},{"💀","skull death"},{"👻","ghost"},
    {"🤖","robot"},{"🤡","clown"},{"😴","sleeping zzz"},{"🥱","yawn bored"},
    {"😷","mask sick"},{"🤒","sick ill"},{"😵","dizzy"},{"🤯","mind blown"},
    {"😳","flushed embarrassed"},{"🥺","pleading puppy eyes"},
    {"😺","cat smile"},{"😸","cat grin"},{"😻","cat heart eyes"},{"😾","cat angry"},
    /* Hands & gestures */
    {"👋","wave hello hi"},{"👍","thumbs up good like"},{"👎","thumbs down bad"},
    {"👌","ok perfect"},{"✌️","peace victory two"},{"🤞","crossed fingers luck"},
    {"🤘","rock horns metal"},{"👏","clap applause"},{"🙌","raised hands celebrate"},
    {"🙏","pray please thank"},{"✍️","writing pen"},{"💅","nail polish"},
    {"👈","left"},{"👉","right"},{"👆","up"},{"👇","down"},{"✊","fist"},
    /* Animals */
    {"🐶","dog puppy"},{"🐱","cat kitten"},{"🐭","mouse"},{"🐰","rabbit bunny"},
    {"🦊","fox"},{"🐻","bear"},{"🐼","panda"},{"🐨","koala"},{"🐯","tiger"},
    {"🦁","lion"},{"🐮","cow"},{"🐷","pig"},{"🐸","frog"},{"🐵","monkey"},
    {"🙈","see no evil"},{"🙉","hear no evil"},{"🙊","speak no evil"},
    {"🐔","chicken"},{"🐧","penguin"},{"🦅","eagle"},{"🦉","owl"},{"🦋","butterfly"},
    {"🐢","turtle"},{"🐍","snake"},{"🦎","lizard"},{"🦕","dinosaur sauropod"},
    {"🦖","t-rex dinosaur"},{"🐙","octopus"},{"🦈","shark"},{"🐬","dolphin"},
    {"🐳","whale"},{"🦓","zebra"},{"🐘","elephant"},{"🦒","giraffe"},
    {"🐺","wolf"},{"🦄","unicorn"},{"🦔","hedgehog"},{"🐾","paw prints"},
    {"🐝","bee"},{"🐛","worm caterpillar"},{"🐞","ladybug"},{"🐜","ant"},
    /* Nature */
    {"🌸","cherry blossom flower pink"},{"🌺","hibiscus flower red"},
    {"🌹","rose flower"},{"🌻","sunflower yellow"},{"🌼","blossom flower"},
    {"🍀","four leaf clover luck"},{"🌿","herb leaves"},{"🌱","seedling plant"},
    {"🌲","tree evergreen"},{"🌳","tree"},{"🍁","maple leaf autumn"},
    {"🍄","mushroom"},{"🌾","wheat rice"},{"☀️","sun sunny"},{"🌙","moon night"},
    {"⭐","star"},{"🌟","star glowing"},{"✨","sparkles magic"},{"⚡","lightning bolt"},
    {"🔥","fire flame hot"},{"💧","water drop"},{"🌊","wave ocean sea"},
    {"🌈","rainbow"},{"❄️","snowflake cold"},{"🌍","earth globe world"},
    /* Food */
    {"🍎","apple red"},{"🍊","orange"},{"🍋","lemon"},{"🍇","grapes"},
    {"🍓","strawberry"},{"🍉","watermelon"},{"🍌","banana"},{"🍒","cherries"},
    {"🥑","avocado"},{"🥕","carrot"},{"🌽","corn"},{"🍕","pizza"},
    {"🍔","burger hamburger"},{"🍟","fries chips"},{"🌮","taco"},{"🍜","ramen noodles"},
    {"🍣","sushi"},{"🍩","donut"},{"🍪","cookie"},{"🎂","birthday cake"},
    {"🍰","cake slice"},{"🧁","cupcake"},{"🍫","chocolate"},{"🍬","candy"},
    {"☕","coffee hot"},{"🍵","tea"},{"🍺","beer"},{"🥂","champagne toast"},
    /* Objects */
    {"📱","phone mobile"},{"💻","laptop computer"},{"⌨️","keyboard"},
    {"🖥️","desktop monitor"},{"📷","camera photo"},{"📺","tv television"},
    {"🎮","game controller"},{"💾","floppy disk save"},{"🔋","battery"},
    {"💡","bulb idea light"},{"🔑","key"},{"🔒","locked secure"},{"🔓","unlocked"},
    {"🔨","hammer tool"},{"🔧","wrench tool"},{"⚙️","gear settings cog"},
    {"🗑️","trash delete"},{"📦","box package"},{"✉️","envelope email"},
    {"📝","memo note"},{"📋","clipboard"},{"📌","pushpin"},{"📎","paperclip"},
    {"✂️","scissors cut"},{"🔍","search magnify"},{"📊","chart graph"},
    {"📅","calendar"},{"⏰","alarm clock"},{"🔔","bell notification"},
    {"🎁","gift present"},{"🎉","party celebrate confetti"},{"🎈","balloon"},
    {"🏆","trophy winner"},{"🥇","gold medal"},{"🎯","dart target"},
    /* Hearts */
    {"❤️","red heart love"},{"🧡","orange heart"},{"💛","yellow heart"},
    {"💚","green heart"},{"💙","blue heart"},{"💜","purple heart"},
    {"🖤","black heart"},{"🤍","white heart"},{"💔","broken heart"},
    {"💕","two hearts"},{"💖","sparkling heart"},{"💘","heart arrow"},
    /* Travel */
    {"🏠","house home"},{"🏢","building office"},{"🏦","bank"},{"🏫","school"},
    {"🚗","car red"},{"🚕","taxi"},{"🚌","bus"},{"🚑","ambulance"},{"🚒","fire truck"},
    {"✈️","airplane flight travel"},{"🚀","rocket space"},{"🚂","train"},
    {"🚢","ship boat"},{"🚲","bicycle bike"},{"🏍️","motorcycle"},
    /* Misc symbols */
    {"💯","hundred perfect"},{"✅","check ok green"},{"❌","cross wrong"},
    {"❓","question"},{"❗","exclamation"},{"♾️","infinity"},
    {"🔴","red circle"},{"🟢","green circle"},{"🔵","blue circle"},
    {"⚫","black circle"},{"⚪","white circle"},
    {"💬","speech bubble"},{"💭","thought bubble"},{"💤","sleep zzz"},
    {"💥","boom explosion"},{"🌀","cyclone spin"},{"🚩","flag red"},
    {NULL, NULL}
};

/* ── Symbols ─────────────────────────────────────────────────────────── */
static const Glyph SYMBOLS[] = {
    /* Math */
    {"±","plus minus"},{"×","multiply times"},{"÷","divide"},{"∞","infinity"},
    {"∑","sum sigma"},{"∏","product"},{"∫","integral"},{"∂","partial derivative"},
    {"√","square root"},{"π","pi math"},{"∆","delta change"},{"∇","nabla"},
    {"∀","for all"},{"∃","exists"},{"∈","element of in"},{"∉","not element"},
    {"⊂","subset"},{"⊃","superset"},{"∪","union"},{"∩","intersection"},
    {"≈","approximately"},{"≠","not equal"},{"≤","less equal"},{"≥","greater equal"},
    {"≡","identical"},{"∅","empty set"},{"∝","proportional"},
    {"ℕ","natural numbers"},{"ℤ","integers"},{"ℝ","real numbers"},{"ℂ","complex"},
    /* Arrows */
    {"→","right arrow"},{"←","left arrow"},{"↑","up arrow"},{"↓","down arrow"},
    {"↔","left right"},{"↕","up down"},{"↗","northeast"},{"↘","southeast"},
    {"↙","southwest"},{"↖","northwest"},{"⇒","implies double right"},
    {"⇐","double left"},{"⇔","iff double"},{"⇑","double up"},{"⇓","double down"},
    {"➡","right solid"},{"⬅","left solid"},{"⬆","up solid"},{"⬇","down solid"},
    {"↩","return hook"},{"↪","right hook"},{"↺","counterclockwise"},{"↻","clockwise"},
    /* Currency */
    {"€","euro"},{"£","pound sterling"},{"¥","yen yuan"},{"₹","rupee"},
    {"₿","bitcoin crypto"},{"$","dollar"},{"¢","cent"},{"₩","won korean"},
    {"₽","ruble"},{"₺","lira"},{"₫","dong"},{"₱","peso"},{"₪","shekel"},
    /* Punctuation */
    {"©","copyright"},{"®","registered"},{"™","trademark"},{"°","degree"},
    {"§","section"},{"¶","paragraph"},{"†","dagger"},{"•","bullet"},
    {"…","ellipsis"},{"—","em dash"},{"–","en dash"},{"¡","inverted exclamation"},
    {"¿","inverted question"},{"¬","not"},{"«","guillemet left"},{"»","guillemet right"},
    /* Keyboard */
    {"⌘","command apple"},{"⌥","option alt"},{"⇧","shift"},{"⌫","backspace delete"},
    {"↵","return enter"},{"⇥","tab"},{"⎋","escape esc"},
    /* Geometric */
    {"●","filled circle"},{"○","white circle"},{"■","filled square"},{"□","white square"},
    {"▲","triangle up"},{"▼","triangle down"},{"◆","diamond filled"},{"◇","diamond"},
    {"★","star filled"},{"☆","star outline"},{"♦","diamond suit"},{"♣","club suit"},
    {"♠","spade suit"},{"♥","heart suit"},{"✓","check tick"},{"✗","cross wrong"},
    {"☑","checkbox checked"},{"☐","checkbox empty"},
    /* Greek */
    {"α","alpha"},{"β","beta"},{"γ","gamma"},{"δ","delta"},{"ε","epsilon"},
    {"θ","theta"},{"λ","lambda"},{"μ","mu micro"},{"π","pi"},{"σ","sigma"},
    {"τ","tau"},{"φ","phi"},{"ω","omega"},{"Ω","omega uppercase"},{"Σ","sigma sum"},
    {"Δ","delta uppercase"},{"Π","pi product"},{"Λ","lambda uppercase"},
    /* Misc */
    {"♻","recycle"},{"⚠","warning"},{"☯","yin yang"},{"☮","peace"},
    {"♪","music note"},{"♫","music notes"},{"♬","music beamed"},
    {"☀","sun"},{"☁","cloud"},{"☂","umbrella rain"},{"❄","snowflake"},
    {"☃","snowman"},{"✝","cross"},{"✡","star of david"},{"☪","crescent star"},
    {NULL, NULL}
};

#endif /* EMOJI_DATA_H */
