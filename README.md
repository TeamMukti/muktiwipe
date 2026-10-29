# muktiwipe
@ By Team Mukti

**muktiwipe** — le libérateur de dossiers et de fichiers pour Windows. Un utilitaire en ligne
de commande **multithread** qui supprime rapidement n'importe quel arbre de dossiers
— ou fichier isolé — y compris les éléments protégés (ACL hostiles, fichiers
verrouillés, propriété TrustedInstaller) — en une seule commande, depuis le
terminal ou le menu contextuel.

> *« mukti »* (संस्कृतम्) : la libération.

**Bilingue** : les messages s'affichent en français ou en anglais selon la
langue de Windows (détection automatique via `GetUserDefaultUILanguage`),
et `--lang=fr` / `--lang=en` permet de forcer la langue.

Basé sur [byenow](https://iobureau.com/byenow) d'Alexander Pankratov
(IO Bureau SA) — moteur d'origine réutilisé, bibliothèque runtime
`libp` réimplémentée, mode de suppression forcé et intégration au menu
contextuel ajoutés par **Team-Mukti**.

> **Compiler le projet ? Rien de plus simple :** double-cliquez sur
> [`build.bat`](build.bat) — c'est tout. Le script détecte Visual Studio
> tout seul et produit `muktiwipe.exe` (aucune DLL requise).
> Détails et variante MinGW dans la section [Compilation](#compilation).

## Pourquoi muktiwipe ?

`rmdir /s /q` supprime les dossiers un par un, sur un seul thread, et
abandonne au premier refus d'accès.

 **Ce que fait muktiwipe ** :

- énumère les dossiers par lots via l'API NT (`NtQueryDirectoryFile`),
- répartit la suppression des fichiers sur plusieurs threads (un par cœur
  logique par défaut),
- supprime chaque dossier dès que son contenu est terminé, sans attendre la
  fin de l'arbre,
- et en mode `--force` / `--trusted`, reprend possession des éléments
  récalcitrants au lieu d'échouer.

Résultat : un dossier de plusieurs milliers de fichiers disparaît en une
fraction de seconde.

## Utilisation

```
muktiwipe.exe [options] <dossier>

  -p --preview           énumère le contenu sans rien supprimer
  -s --staged            énumère d'abord tout l'arbre, puis supprime

  -j --journal           demande confirmation et attend Entrée à la fin
  -x --yolo              ne bloque pas les chemins restreints
  -1 --one-liner         progression sur une seule ligne
  -b --show-bytes        affiche les compteurs en octets
  -e --list-errors       liste toutes les erreurs à la fin
  -y --yes               supprime sans demander (comportement par défaut)
  -f --force             prend possession, déverrouille et supprime
                         « quoi qu'il arrive »
  -T --trusted           comme --force, en se relançant élevé (UAC)
                         si nécessaire

  -o --omni-delete       autorise <dossier> à désigner un fichier
  -k --keep-folder       ne supprime pas le dossier lui-même

  -t --threads <n>       nombre de threads (défaut : nombre de cœurs)
  -n --delete-ntapi      supprime les fichiers via NtDeleteFile
  --lang=fr|en           langue des messages (défaut : langue du système)
```

Exemples :

```
muktiwipe.exe -p D:\temp\node_modules       # aperçu (rien n'est supprimé)
muktiwipe.exe D:\temp\node_modules          # supprime directement (défaut)
muktiwipe.exe -j D:\temp\node_modules       # demande confirmation avant
muktiwipe.exe -f "D:\dossier corrompu"      # mode forcé (ACL, verrous, ...)
muktiwipe.exe -x -T "C:\Windows\..."        # mode Trusted (élévation auto)
muktiwipe.exe -s -t 16 \\serveur\partage\gros_dossier  # stockage réseau
```

Par défaut, muktiwipe **supprime directement** sans poser de question.
Avec `--journal`, il demande confirmation avant d'agir (`Type y = yes /
n = no`) et, lancé dans sa propre fenêtre (menu contextuel, double-clic),
affiche la synthèse puis attend `Entrée` avant de fermer la fenêtre ;
lancé depuis un terminal, il rend la main normalement. `-y` reste accepté
pour la compatibilité (c'est le comportement par défaut).

## Modes forcé (`-f`) et Trusted (`-T`)

Équivalent intégré des recettes « takeown + icacls + handle.exe + taskkill » :
sans ces options, un élément dont la suppression échoue produit simplement
une erreur ; avec `--force` (ou `--trusted`), par élément en échec (accès
refusé, violation de partage), muktiwipe :

1. termine les processus qui verrouillent l'élément via l'**API Restart
   Manager** (native, plus précise que handle.exe) — sauf les processus
   critiques du système (smss, csrss, winlogon, services, lsass, ...)
   et muktiwipe lui-même, chaque kill étant affiché `[unlock] ...` ;
2. s'attribue la propriété au profit du groupe Administrateurs
   (équivalent `takeown /A`) ;
3. remplace la liste de contrôle d'accès par un contrôle total pour
   Everyone (équivalent `icacls *S-1-1-0:F`), puis réessaie.

La récupération s'applique aussi à l'**ouverture des dossiers pour
l'énumération** (une ACL hostile n'empêche plus le scan). Les attributs
lecture-seule/système/caché sont nettoyés au passage. Si un élément
résiste toujours, il est signalé et le code de sortie est >= 10 — la
vérification finale du succès se fait donc par le code de sortie.

`--force` ne tente aucune élévation : les éléments protégés par
TrustedInstaller ne peuvent pas être pris de possession depuis un
processus non élevé (le privilège SeTakeOwnership n'existe que dans un
jeton élevé). C'est le rôle de `--trusted` : il implique `--force` et,
si le processus courant n'a pas le privilège de prise de possession,
muktiwipe **se relance lui-même élevé** (invite UAC), attend la fin et
propage le code de sortie — le remplacement direct du wrapper nsudo.
`--no-elevate` désactive cette relance (utile pour les scripts et les
tests). Si l'utilisateur refuse l'UAC, muktiwipe continue en mode dégradé
avec un avertissement.

Limites connues : un dossier qui est le répertoire courant d'un processus
est contourné (muktiwipe change son propre répertoire courant), mais le
processus concerné n'est pas tué s'il n'apparaît pas dans le Restart
Manager. Les processus protégés (TrustedInstaller, etc.) ne peuvent pas
être terminés — mais muktiwipe élevé peut prendre possession de leurs
fichiers, ce qui suffit pour les supprimer.

## Codes de sortie

| Code | Signification |
|-----:|---------------|
| 0 | succès |
| 1 | annulé (Ctrl-C) |
| 10+ | succès avec erreurs (10 + log10 du nombre d'erreurs) |
| 50/51/52 | pas de chemin / argument invalide / confirmation refusée (avec `--journal`) |
| 60–65 | erreur de chemin (introuvable, racine, restreint, ...) |

## Sécurité

Par précaution, muktiwipe **refuse de supprimer** le dossier Windows et la
racine des profils (`C:\Users`) : l'option `--yolo` est requise pour
outrepasser ce blocage. La vérification est faite sur le chemin **complet
résolu**, donc un chemin relatif ou une notation indirecte ne permet pas de
contourner la protection. Les jonctions et liens symboliques ne sont **pas**
suivis : seul le lien lui-même est supprimé. La racine d'un lecteur n'est
pas une cible valide.

## Compilation

Avec Visual Studio (chaîne d'outils MSVC) :

```
build.bat          # produit muktiwipe.exe (Release x64)
build.bat clean    # supprime les fichiers intermédiaires
```

Depuis l'Explorateur, un simple **double-clic sur `build.bat`** suffit.
Le script détecte Visual Studio via `vswhere` ; il faut la charge de
travail « Desktop development with C++ ». L'exécutable produit est
statique (`/MT`) : aucune DLL n'est requise.

Alternative MinGW-w64 :

```
windres muktiwipe.rc -O coff -o muktiwipe_res.o
g++ -O3 -std=c++14 -municode -static-libgcc -static-libstdc++ ^
    -o muktiwipe.exe wmain.cpp muktiwipe.cpp ultra_machine.cpp folder.cpp ^
    delete_file.cpp utils.cpp libp\*.cpp muktiwipe_res.o -lrstrtmgr -ladvapi32
```

Pour lancer la suite de tests fonctionnels (18 tests) :

```
powershell -NoProfile -ExecutionPolicy Bypass -File tests\muktiwipe_test.ps1
```

## Intégration au menu contextuel

`muktiwipe.exe -x -T` remplace intégralement un script de type
« Trusted Fast Delete » (nsudo, attrib, takeown, icacls, handle.exe,
taskkill, rmdir, PowerShell, LockHunter). Un fichier `menu_contextuel.reg`
prêt à l'emploi est fourni :

```
[HKEY_CLASSES_ROOT\AllFilesystemObjects\shell\Fast_delete]
@="Suppression Rapide (mode Trusted)"
"Icon"="imageres.dll,-54"
;
[HKEY_CLASSES_ROOT\AllFilesystemObjects\shell\Fast_delete\command]
@="\"C:\\Windows\\System32\\muktiwipe.exe\" -x -T -o \"%1\""
```

- Adapter le chemin vers muktiwipe.exe (ou utiliser `%SystemDrive%\...`).
- `-x` est requis pour pouvoir cibler les dossiers sous `C:\Users` et
  `C:\Windows` ; `-T` gère l'élévation lui-même (invite UAC à chaque
  utilisation).

## Organisation du code

- `wmain.cpp` — point d'entrée, protection SEH/C++ et gestionnaire d'assertions
- `muktiwipe.cpp` — application : analyse des arguments, élévation, affichage, orchestration (commencer la lecture ici, `wmain_app()`)
- `ultra_machine.cpp` — moteur d'énumération/suppression en 3 phases et pool de threads
- `folder.cpp` — arborescence des dossiers et comptage du travail restant
- `delete_file.cpp` — suppression Win32 ou NT API, attributs HSRO, récupération du mode forcé
- `utils.cpp` — formatage des tailles/durées, description des erreurs, chemins restreints
- `libp/` — bibliothèque de support (runtime du programme) :
  `_scan_folder_nt` (énumération NT), `_simple_work_queue` (pool de threads),
  `_elpify` (chemins étendus `\\?\` au-delà de 260 caractères), `_security`
  (prise de possession/DACL), `_unlock` (Restart Manager), `_console`,
  `_filesys`, `_system_api`, `_cpu_info`, `api_error`, `atomic`, `string_utils`, ...
- `tests/muktiwipe_test.ps1` — suite de tests fonctionnels

## Améliorations par rapport au byenow d'origine

- **Mode -T `--force` / `--trusted`** : prise de possession, contrôle total,
  nettoyage d'attributs et déverrouillage par Restart Manager, avec
  nouvelle tentative par élément en échec — remplace les scripts batch
  externes ; `--trusted` s'auto-élève (UAC).
- **Blocage corrigé** : une jonction/lien symbolique dans l'arbre gelait la
  suppression en mode par défaut (le compteur d'éléments restants
  n'atteignait jamais zéro) ; le lien est désormais supprimé sans être suivi.
- **Sécurité renforcée** : le contrôle des chemins restreints s'applique
  après résolution du chemin complet (un chemin relatif ne contourne plus la
  protection), avec vérification de la frontière de composant, et cible le
  véritable dossier Windows/profils au lieu de `C:\` codé en dur.
- **Bibliothèque libp réimplémentée** : le source publié d'byenow dépendait
  d'une bibliothèque non distribuée ; elle est fournie ici, le projet
  compile de manière autonome.
- **Compatibilité** : `RtlDosPathNameToNtPathName_U` en repli si la
  variante `_WithStatus` (Win10 1703+) est absente ; formats printf
  portables ; curseur de la console restauré à la sortie.

## Licence

Le moteur d'origine est l'œuvre d'[Alexander Pankratov / IO Bureau SA]
(https://iobureau.com/byenow), distribué sous licence BSD-2 clauses avec
condition Commons Clause (voir [LICENSE](LICENSE)) : le code reste libre à
l'usage et à la modification, mais ne peut pas être vendu. Les copyright
d'origine sont conservés dans les fichiers sources ; les modifications
apportées par Team-Mukti (mode forcé/trusted, libp, correctifs) sont
soumises aux mêmes termes.
