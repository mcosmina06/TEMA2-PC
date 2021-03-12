MANDRU COSMINA
323CB
 



 							Tema 2 PC

 	Tema am dezvolat-o ep scheletul laboratorului 8.
 	In healpers.h se gasesc structurile pe care le-am utilizat in realizarea temei
si 2 funcntii mai ample. Functia check_message este de tip bool care returnează
true dacă toate argumentele date pentru fiecare comandă sunt valide. Pentru ambele
comenzi, subscribe si unsubscribe, se retine numele intr-o variabila. Cel de-al doilea 
parametru este topicul care nu trebuie să depaseasca lungimea de 50 de caractere. 
În cazul în care comanda este de tip subscribe se verifica si urmatorul parametru 
care nu trebuie sa fi altceva in afara de 0 sau 1. În cazul în care toate argumentele 
date sunt valide functia va intoarce true, in caz contrar se vor afisa mesaje de eroare
specifice. Functia sendMessage are drept rol filtrarea mesajului in functie de cazurile
impuse si construirea unui mesaj in functie de structura construita pentru a putea fi
transmis mai departe clientilor ce sunt abonati si afisat. Tot aici in cazul in care
un client este inactiv dar este aboant la topicul respectiv si are argumentul SF setat
la 1, se va salva mesajul in history.

    In subscriber verific corectitudinea parametrilor și in cazul in care nu corespund
condițiilor impuse sunt intoarse mesaje adecvate. Se creaza conexiunea cu serverul
și setam campurile despre socketul TCP. Se dezactiveaza algoritmul lui Neagle imediat
dipa conectarea la server. Se seteaza file descriptorii socketilor atat pentru server
cat si pentru terminal, folositi ulterior la funnctia de select. Daca s-a primti un
mesaj de la consola si acesta este "exit" atunci conexiunea se incheie. In caz contrar
se apeleaza functia de check_message pentru a vedea daca mesajul este unul de format 
acceptat si daca este subscribe sau unsubscribe si se afiseaza in consola mesaje 
corespunzatoarte. In cazul in care se primeste un mesaj de la server se verfica campul
de type pentru a sti cum sa fac printarea.

    In server am deschis deschide 2 sockeți, unul de TCP pe care se aștepta conexiuni pe
adresele IP disponibile din sistem și u socket de tip UDP. La fel ca si in subscriber sunt
setati file descriptorii fisierelor, luandu-se mereu cel maxim. In caaul in care se primesc 
date pe socketul de TCP inseamna ca se doreste realizarea unei noi conexiuni, astfel se
parcurge vectorul de clienti si se cauta dupa id pentru a se vedea daca exista deja
acel client dar este inactiv. Daca da atunci se trimit mesajele salvate in hisotry.
Daca nu este gasit atunci clientul nou este introdus, afisandu-se si un mesaj corespunzator.
Daca se prjmesc date pe socketul UDP se va apela functia de filtrare a mesajelor UDP, 
sendMessage. Si ultimul caz, cel care se primesc date pe socketul TCP in cazul in care
nu s-a primit nimic la recv inseamna ca, conexiune a fost inchisa si vom seta clientul
cu fd-ul respectiv sa fie inactiv. Daca este o comanda de unsubscribe se cauta in vectorul
de topicuri al clientului pentru a vedea daca exista. In caz negativ se afiseaza un mesaj de 
eroare, acesa este eviatat doar daca SF - urile topicului din vector si cu cel primit sunt 
diferite, atunci realizandu-se o actualizare. In cazul de subscribe, este adaugat topicul
doar daca nu exista deja.

	Am tratat si aspectul legat de SF. In cazul in care se intrerupe conexiunea cu un anumit
client, acesta nu este sters din vector ci i se seteaza campul active pe false. Cant timp acesta
este inactiv i se stocheaza mesajele in history si le va primi imediat ce conexiunea a fost 
restabilita.
