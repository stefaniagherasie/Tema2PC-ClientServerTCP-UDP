# Tema2PC-ClientServerTCP-UDP
[Tema2 Protocoale de Comunicatii (2019-2020, seria CB)] 


Scopul temei este realizarea unei aplicatii care respecta modelul client-server
pentru gestiunea mesajelor. Serverul va realiza legatura dintre clientii din
platforma. Clientii TCP se pot conecta la server si pot trimite acestuia mesaje
de tip subscribe/unsubscribe. Clientii UDP trimit mesaje in format predefinit
la server, care le transmite apoi la subscriber. <br>
Enunt: [aici](https://acs.curs.pub.ro/2019/pluginfile.php/70988/mod_resource/content/1/Tema_2_Protocoale_2019_2020.pdf)

## Compilare si Rulare
> ```shell
>      make
> ```    
> ```shell 
>     ./server <port>
>     ./subscriber <client_id> <ip_server> <port>
> ```
                    
                    
## Generare Mesaje UDP
> Comenzi valide cu care se poate face rularea unui client, dacă serverul rulează local pe portul 8080:
> ```shell
>     ► python3 udp_client.py 127.0.0.1 8080
>     ► python3 udp_client.py --mode manual 127.0.0.1 8080
>     ► python3 udp_client.py --source-port 1234 --input_file three_topics_payloads.json
>                             --mode random --delay 2000 127.0.0.1 8080
> ```

## Implementare
Pentru inceput s-au creat niste structuri care sa retina mesajele (`udp_msg` si 
`tcp_msg`), care sa retina un abonament la un topic si o structura de tip client.


#### 1. SERVER

Pentru a putea comunica cu cele doua tipuri de clienti, se deschid atat 
un socket `TCP` (tcp_sockfd), cat si unul `UDP` (udp_sockfd). Pentru a putea conecta
la server mai multi subscriberi, se foloseste multiplexarea. Astfel, se adauga 
la multimea de descriptori de citire, folositi de apelul `select()`, socketul TCP, 
socketul UDP, respectiv 0 (pentru citirea de la stdin). <br>
Intr-o structura 
repetitiva, pentru fiecare numar pana la descriptorul cu cea mai mare valoare,
se verifica daca acesta apartine multimii descriptorilor de citire, situatie
in care se trateza mai multe cazuri (daca numarul este 0, tcp_sockfd sau udp_sockfd).

- Daca descriptorul este 0, se citeste de la tastatura o comanda. In cazul in care 
aceasta este `exit`, se inchide serverul si conexiunea la toti subscriberi.

- Daca descriptorul este cel corespunzator socketului TCP un subscriber a trimis
o cerere de conexiune la server. Acestuia ii este acceptata conexiunea. Pentru
a retine toti clientii se foloseste vectorul `clients` in care sunt stocati
clientii. Acestia sunt caracterizati de id, fd, state (activ sau inactiv) si de
o lista cu mesajele primite cand clientul este inactiv. Daca clientul este nou, 
acesta se adauga in vector, iar daca acesta se reconecteaza se activeaza 
(se face state = true) si ii se trimit mesajele stocate pentru topicurile la 
care era abonat cu SF = 1.

- Daca descriptorul este cel corespunzator socketului UDP un client a trimis un
mesaj care trebuie transmis mai departe la clientii TCP abonati. Se obtine
topicul mesajului si acesta se cauta in lista `topics` (care contine structuri de 
tipul `subscription`). Se stocheaza separat clientii abonati la acelasi topic cu 
SF diferit. Mesajul primit de la UDP se primite doar la clientii abonati la
topicul dat, iar pentru cei inactivi si cu SF = 1 se stocheaza aceste mesaje
intr-o lista.

- Daca descriptorul nu este in niciunul din cazurile de mai sus, inseamna ca
s-a primit un mesaj de la clientul TCP. Acesta este de tipul `tcp_msg` si 
contine o comanda de subscribe/unsubscribe. Daca se primeste un topic inexistent
pana atunci, acesta se adauga, fiind adaugat si clientul la lista acestuia de 
subscriberi. Daca topicul exista, se adauga doar clientul. Pentru unsubscribe 
se elimina clientul din lista de subscriberi a topicului dorit.


#### 2. SUBSCRIBER

Pentru a putea comunica cu serverul, se deschide un socket TCP pentru acesta.
In mod asemanator cu serverul, adaug la descriptorii de citire 0 si sockfd.

- Daca descriptorul este 0 se citeste de la tastatura comanda de `exit`, `subscribe`,
`unsubscribe`. Pentru mesajele de abonare, se creeaza o structura de tipul `tcp_msg`.
Se obtin datele necesare pentru a forma mesajul si se tine cont de cazurile in
care se introduc comenzi invalide. Mesajul de abonare se trimite la server.

- Daca descriptorul este cel al socketului de TCP inseamna ca s-a primit un
mesaj de la server, care provine de la clientul UDP. Acesta este prelucrat 
pentru tinand cont de formatul prestabilit in enuntul temei. Acesta este afisat
la stdout asa cum este specificat.

Subscriberul are o lista de topicuri la care e abonat, care este folosita pentru
a ne asigura ca acesta nu se aboneaza la acelasi topic de mai multe ori sau
sa incerce sa se dezaboneze de la un topic la care nu era abonat.

## Mentiuni

Am tratat urmatoarele cazuri de eroare: orice tip de eroare aparuta a conexiunea
dintre server si clienti, comenzi invalide la mesajele de abonare, id-ul clientului
avand mai mult de 10 caractere, topicul avand mai mult de 50 de caractere, SF diferit
de 0/1, comenzi invalide la server, subscriberul incearca se se reaboneze la topic,
clientul se reaboneaza cu alt SF, subscriberul se dezaboneaza de la un topic la care
nu era abonat.
