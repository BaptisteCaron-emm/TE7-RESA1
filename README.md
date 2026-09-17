# TE7-RESA1 — Projet de programmation réseau

Ce projet consiste à développer en binôme une application de messagerie instantanée client/serveur en C. Elle doit permettre des échanges entre deux utilisateurs, entre plusieurs utilisateurs et, selon les jalons, des échanges de fichiers. Le projet a pour objectif principal de mettre en pratique l'API sockets POSIX et les communications TCP/IP.

Vous construirez l'application progressivement. **Un jalon doit être entièrement terminé et soumis avant de commencer le suivant.** Vous avez 14h40 en séances encadrées. L'intégralité du projet est réalisable sur ce créneau là. Vous ne devez PAS travailler à la maison sur ce projet, il est UNIQUEMENT à réaliser en séance.

## Parcours du projet

Lisez d'abord les règles communes ci-dessous, puis avancez dans l'ordre des jalons.

| Étape | Objectif | Spécification |
| --- | --- | --- |
| Jalon 1 | Client/serveur TCP IPv4, écho et multi-clients | [jalon1.md](jalon1.md) |
| Jalon 2 | Utilisateurs, pseudos et messagerie | [jalon2.md](jalon2.md) |
| Jalon 3 | Transfert de fichiers pair à pair | [jalon3.md](jalon3.md) |
| Jalon 4, extension | Salons de discussion | [jalon4.md](jalon4.md) |

Les [ressources](ressources.md) regroupent les conseils de débogage et le rappel de C fourni avec le sujet. Le [contrat de protocole](protocole.md) définit le format des trames, les limites et les erreurs de communication à respecter.

Le jalon 4 est une extension. Il ne doit être abordé qu'après validation des fonctionnalités obligatoires. Le jalon 3 nécessite que chaque client puisse à la fois maintenir sa connexion avec le serveur et écouter temporairement une connexion pair à pair.

## Dépôt de départ

Après avoir récupéré le dépôt, vous y trouverez :

- `sample-jalon1/` et `sample-jalon2/` : squelettes de code et Makefiles pour les jalons 1 et 2 ;
- `travail/` : répertoire de développement. C'est ce répertoire qui est évalué pour les soumissions intermédiaires ;
- `rendu_final/` : répertoire destiné au rendu final ;
- `info.txt` : à compléter avec les noms, prénoms et logins GitHub du binôme.

## Dépôt GitHub

Pour pousser votre code, créez un *Personal Access Token* GitHub en suivant [le guide officiel](https://docs.github.com/en/authentication/keeping-your-account-and-data-secure/creating-a-personal-access-token). Lors d'un `git push`, utilisez ce jeton à la place du mot de passe.

## Soumettre un jalon

Chaque jalon est soumis sur GitHub sous la forme d'une *release*, qui fige l'état du dépôt à un instant donné.

1. Placez le code attendu dans le bon répertoire : `travail/` pour les jalons intermédiaires, `rendu_final/` pour le rendu final.
2. Créez une release nommée `jalonx`, où `x` est le numéro du jalon.
3. Publiez la release. Une release laissée à l'état de brouillon est ignorée.
4. N'ajoutez pas de fichiers joints à la release : ils ne sont pas pris en compte lors de l'évaluation.

Consultez au besoin [la documentation GitHub sur la création de releases](https://docs.github.com/repositories/releasing-projects-on-github/managing-releases-in-a-repository). Les soumissions sont analysées par [JPlag](https://github.com/jplag/JPlag) et [MOSS](https://theory.stanford.edu/~aiken/moss/).

### Échéances indiquées dans le sujet

| Livrable | Échéance |
| --- | --- |
| Jalon 1 | 14 septembre, 23 h 59 |
| Jalon 2 | 19 septembre, 23 h 59 |
| Jalons 3 et 4, et rendu final | 30 septembre, 23 h 59 |


## Rendu final

Copiez le code de `travail/` vers `rendu_final/`. Le rendu final doit impérativement compiler avec un Makefile, sans erreur ni avertissement, sur les machines de l'ENSEIRB-MATMECA. Vous pouvez repartir des Makefiles fournis dans `sample-jalon1/` et `sample-jalon2/`.

Pour développer et tester sur les machines de l'école, connectez-vous à `ssh.enseirb.fr`, puis à une machine de TP. La commande `netgroup <numéro_de_salle>` permet d'identifier les machines disponibles, par exemple `netgroup I101`.

## Évaluation

L'évaluation porte notamment sur :

- le respect des modalités et échéances de soumission
- l'implémentation des fonctionnalités demandées
- la robustesse face à des messages non implémentés ou malformés, côté client comme côté serveur
- la libération de la mémoire et la fermeture des sockets, notamment vérifiées avec `valgrind` et `lsof`.

### Bonus et malus

| Critère | Effet |
| --- | --- |
| Consignes non respectées : échéances, `info.txt`, Makefile, erreurs ou avertissements importants à la compilation | -2 points |
| Mauvaise utilisation des primitives de lecture et d'écriture sur sockets | -2 points |
| Code peu lisible : indentation, `main()` trop long, absence de factorisation ou de fonctions | -2 points |
| Mémoire allouée mais non libérée | -2 points |
| Descripteurs de sockets ou fichiers non fermés | -2 points |
| Fonctionnement en IPv4 et IPv6 | +0,25 point |

Chaque réalisation de jalon donne des points dans la note finale. Pour obtenir les points d'un jalon, toutes les fonctionnalités et spécificités des jalons précédents doivent être implémentées sans erreur.

*Surprise* : ajoutez des fonctionnalités au-delà des jalons 1 à 4.
