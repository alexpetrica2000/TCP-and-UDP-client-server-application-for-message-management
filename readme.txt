	Implementarea temei a durat in jur de 15 ore,
timp in care am invatat cum lucreaza socketii, cum se realizeaza
multiplexarea I/O, cum lucreaza o aplicatie de tipul client server
si multa altele.
	Pentru inceput voi vorbi despre structurile implementate. Astfel,
in helpers.h sunt mentionate structurile utilizate. proto_tcp_msg este
protocolul aplicatie peste TCP. Acesta contine un camp dim, ce reprezinta
dimensiunea mesajului in octeti, un buffer de dimensiune 1600 octeti 
si un id pe 10 octeti. Acestea sunt utilizate pentru a stii cat
sa citesc, cat se trimite pentru a nu avea probleme la trunchieri /
concatenari sau orice alte probleme care ar avea loc in timpul transportului.
Am folosit o lista simplu inlantuita care contine topic si 2 referinte
catre primul si ultimul nod al listei de noduri, un nod fiind reprezentat
ca un sir de caractere data reprezentand id-ul, 4 octeti pentru socket sau
SF (adica 0 sau 1, am folosit in paralel pentru a nu crea 2 liste distincte),
cat si referinta catre urmatorul nod al listei. ListSFNode este nodul dintr-o
alta lista, diferita prin faptul ca este folosit pentru a trimite mesajele
clientilor abonati cu sf = 1 ce au fost deconectati cand s-a trimis un mesaj
de care acestia erau interesati. Astfel am id-ul, dimensiunea sirurilor
de caractere (cate mesaje are de primit), vectorul de buffere si referinta
catre nodul urmator. Operatiile pe liste sunt implementate in list.c. Acestea
sunt doar operatii de baza (search, insert, delete, update).
	In server.c, au loc urmatoarele:
- dezactivarea bufferingului la afisare cu setvbuf
- alocarile listelor de clienti activi, a listei
de liste de topicuri si a celei pentru sf
- efectuarea operatiilor initiale pe socketii TCP/UDP (socket, completare 
structura sockaddr_in, bind, listen)
- alcatuirea multimii initiale de file descriptori (initial din stdin, socktcp,
sockudp)
	Apoi pot sa primesc date pe fiecare dintre socketi si verific:
- daca am primit date pe socketul tcp, inseamna ca a venit o cere de conexiune,
pe care serverul o accepta. se face recv pentru a primii id-ul si se adauga
in lista de clienti conectati daca nu exista deja vreun nod care are acelasi id
cu id-ul clientului ce incearca sa se conecteze. In caz contrar inseamna ca 
exista deja un client cu acest id deci se respinge clientul, i se trimite mesaj
de exit pentru a se inchide. Daca clientul a reusit conectarea atunci se pune
problema daca are mesaje de primit (adica daca are subscribe sf = 1 la unul
din topicuri si s-au trimis mesaje intre timp pe care acesta le-ar primii in
acest moment). Daca exista mesaje, le primeste si se reseteaza list_topic_size
pentru userul respectiv, adica se goleste vectorul de siruri.
- daca am primit date de pe socketul udp, conform cerintei, stiu formatul 
acestora. Practic primesc un topic[51], tip date pe un octet si un
continut[1500]. Primesc datele folosind recv intr-un buffer pe 1600 ce apoi
este spart in formatul corespunzator folosind memcpy. Ulterior ne vom forma 
mesajul care va fi trimis catre clientii TCP folosind datele din enunt (topicul
ramane neafectat, se formeaza numarul folosind tipul de date si octetii din
continut). Dupa formarea mesajului avem de aflat caror clienti este destinat
mesajul. Mergem pe lista ce contine topicul trimis de UDP si pentru aceasta
avem lista de noduri de utilizatori. Daca acestia sunt conectati, se formeaza
mesajul tcp (se seteaza dimensiunea ca fiind strlen(buffer) + 1, id-ul este
id-ul preluat din nod, mesajul este bufferul iar socketul pe care va fi trimis
mesajul este preluat din lista de utilizatori conectati). Daca clientul
este deconectat dar are sf = 1 pentru acest topic atunci se face updateSFlist
(practic se copiaza mesajul in vectorul de siruri de caractere), ca mai apoi
clientul sa primeasca mesajul la conectare.
- daca am primit date de la stdin verificam doar daca este exit, caz in care
trimitem tuturor mesaj de exit pentru a se inchide, dupa care ne inchidem si 
noi ca server.
- Altfel am primit date de la socketii client, verificam daca a dat mesaj
de exit, caz in care il stergem din lista de clienti conectati si stergem
socketul din multimea de file descriptori. Daca nu este mesaj de exit
atunci avem ori mesaje de tipul subscribe topic [0-1] sau unsubscribe topic.
Daca este de tip subscribe, preluam si sf-ul, vedem daca este 1 caz
in care daca nu exista nod cu acest id il cream. Iteram prin lista de liste
si cautam potrivirea de topic, la care vom adauga in cadrul acelei liste
clientul ce a dat subscribe cu sf-ul respectiv. Daca este unsubscribe
doar se itereaza din nou prin lista de liste, se gaseste potrivirea de topic
si se sterge clientul din lista respectiva.
	In subscriber.c prima parte este asemanatoare cu cea din server.c, adica
partea de alcatuire a multimii de fd, deschiderea socketului, conectare.
O sa trecem direct la partea in care primim date de la stdin sau de la socketul
cu care suntem conectati. 
- daca primim de la stdin, verificam intai daca este mesaj de exit caz in care
i-am transmite serverului ca urmeaza sa ne deconectam. Daca nu este mesaj de 
acest gen verificam corectitudinea mesajului, adica daca respecta patternul
subscribe topic [0-1] sau unsubscribe topic. Daca nu respecta acest pattern
se ignora mesajul. 
- daca primim date de la socketul cu care suntem conectati, citim intai 
dimensiunea datelor primite (conform protocolului aplicatie peste TCP),
iar apoi in bucla while citim pana cand se atinge aceasta dimensiune (
pentru a fi siguri ca am primit corect si complet mesajul).