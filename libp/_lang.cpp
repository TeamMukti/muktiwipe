/*
 *	libp - a small library of Win32/C++ helpers
 *
 *	Bilingual user messages (English / French).
 */
#include "_lang.h"

#include "_windows.h"
#include "enforce.h"

#include <winnls.h>

//
int g_lang = LANG_EN;

void lang_init()
{
	// primary language id 0x0c is French (LANGID = primary | sublang << 10)
	g_lang = ((GetUserDefaultUILanguage() & 0x3FF) == 0x0c) ? LANG_FR : LANG_EN;
}

//
static const char * table_en[M_count] =
{
	NULL,

	// M_header
	"Fast multithreaded deletion of folders and files on Windows\n",

	// M_syntax
	"Syntax: muktiwipe.exe [options] <folder>\n"
	"\n"
	"  Deletes a folder (or a file, with -o). Similar to 'rmdir /s ...', but multi-threaded.\n"
	"\n"
	"  -p --preview           enumerate contents, but don't delete anything\n"
	"  -s --staged            enumerate contents first, then delete them\n"
	"\n"
	"  -j --journal           ask for confirmation and wait for Enter when done\n"
	"  -x --yolo              don't block deletion in restricted paths\n"
	"  -1 --one-liner         show progress as a single line\n"
	"  -b --show-bytes        show total/deleted byte counts\n"
	"  -e --list-errors       list all errors upon completion\n"
	"  -y --yes               delete without asking (default behavior)\n"
	"  -f --force             take ownership, unlock and delete at all costs\n"
	"  -T --trusted           force, relaunching elevated (UAC) if needed\n"
	"\n"
	"  -o --omni-delete       allow <folder> to point at a file\n"
	"  -k --keep-folder       don't delete the folder itself, just its contents\n"
	"\n"
	"  -t --threads <count>   use specified number of threads\n"
	"  -n --delete-ntapi      use NtDeleteFile to remove files\n"
	"  --lang=fr|en           message language (default: system language)\n"
	"\n"
	"  * By default the thread count is set to the number of CPU cores.\n"
	"    For local folders it doesn't make sense to go above that, but\n"
	"    for folders on network shares raising the thread count may be\n"
	"    a good thing to try, especially for high-latency connections.\n",

	// M_nt_api_missing
	"Failed to locate required NT API entry points.\n",

	// M_scan_buf_max
	"Maximum supported scan buffer size is 64MB.",

	// M_root_not_supported
	"Root of a drive is not supported as a target.",

	// M_restricted
	"Restricted path - %s\n",

	// M_scanning
	"Scanning",

	// M_deleting
	"Deleting",

	// M_staged_tag
	"[staged]",

	// M_col_folders
	"Folders",

	// M_col_files
	"Files",

	// M_col_bytes
	"Bytes",

	// M_col_errors
	"Errors",

	// M_found_h
	"  Found    ",

	// M_deleted_h
	"  Deleted  ",

	// M_found
	"  Found  ",

	// M_deleted
	"  Deleted",

	// M_togo
	"    [%zu to go]",

	// M_cryptic
	"%zu / %zu folders, %zu / %zu files, %s / %s, %zu / %zu errors",

	// M_cryptic_togo
	" - %zu to go",

	// M_err_fullpath
	"Error: failed to get full path name for [%s].\n",

	// M_err_not_found
	"Error: specified path not found - [%s].\n",

	// M_err_generic
	"Error: %s\n",

	// M_path_label
	"Path: [%s]\n",

	// M_err_is_file
	"Error: specified path points at a file - [%s]\n",

	// M_confirm_folder
	"Remove [%s] and all its contents?\n",

	// M_confirm_file
	"Delete [%s] file?\n",

	// M_confirm_prompt
	"Type y = yes / n = no : ",

	// M_cancelled
	"Cancelled.\n",

	// M_completed_hint
	"Completed in %s. To list errors use '--list-errors'.\n",

	// M_completed
	"Completed in %s\n",

	// M_done_in
	" - done in %s\n",

	// M_errors_block
	"Errors:\n",

	// M_no_desc
	"<no description available>",

	// M_press_enter
	"Press Enter to exit...",

	// M_elev_refused
	"WARNING - elevation refused, forcing without administrator rights\n",

	// M_elev_hint
	"(TrustedInstaller-protected files may not be deletable)\n",

	// M_unlocked
	"[unlock] terminated %s (PID %lu)\n",
};

//
static const char * table_fr[M_count] =
{
	NULL,

	// M_header
	"Suppression de dossiers et de fichiers multithread rapide sous Windows\n",

	// M_syntax
	"Syntaxe : muktiwipe.exe [options] <dossier>\n"
	"\n"
	"  Supprime un dossier (ou un fichier, avec -o). Comme 'rmdir /s ...', mais multithread.\n"
	"\n"
	"  -p --preview           énumère le contenu sans rien supprimer\n"
	"  -s --staged            énumère d'abord tout l'arbre, puis supprime\n"
	"\n"
	"  -j --journal           demande confirmation et attend Entrée à la fin\n"
	"  -x --yolo              ne bloque pas les chemins restreints\n"
	"  -1 --one-liner         progression sur une seule ligne\n"
	"  -b --show-bytes        affiche les compteurs en octets\n"
	"  -e --list-errors       liste toutes les erreurs à la fin\n"
	"  -y --yes               supprime sans demander (comportement par défaut)\n"
	"  -f --force             prend possession, déverrouille et supprime à tout prix\n"
	"  -T --trusted           force, en se relançant élevé (UAC) si nécessaire\n"
	"\n"
	"  -o --omni-delete       autorise <dossier> à désigner un fichier\n"
	"  -k --keep-folder       ne supprime pas le dossier lui-même\n"
	"\n"
	"  -t --threads <nombre>  nombre de threads à utiliser\n"
	"  -n --delete-ntapi      supprime les fichiers via NtDeleteFile\n"
	"  --lang=fr|en           langue des messages (par défaut : langue du système)\n"
	"\n"
	"  * Par défaut, le nombre de threads est celui des coeurs du CPU.\n"
	"    Pour des dossiers locaux, inutile de dépasser ce nombre, mais\n"
	"    pour des dossiers sur un partage réseau, l'augmenter peut être\n"
	"    utile, en particulier si la connexion est lente.\n",

	// M_nt_api_missing
	"Impossible de localiser les points d'entrée NT API requis.\n",

	// M_scan_buf_max
	"Taille maximale du tampon de scan : 64 Mo.",

	// M_root_not_supported
	"La racine d'un lecteur n'est pas une cible valable.",

	// M_restricted
	"Chemin restreint - %s\n",

	// M_scanning
	"Analyse",

	// M_deleting
	"Suppression",

	// M_staged_tag
	"[2 passes]",

	// M_col_folders
	"Dossiers",

	// M_col_files
	"Fichiers",

	// M_col_bytes
	"Octets",

	// M_col_errors
	"Erreurs",

	// M_found_h
	"  Trouvés  ",

	// M_deleted_h
	"  Effacés  ",

	// M_found
	"  Trouvés",

	// M_deleted
	"  Effacés",

	// M_togo
	"    [%zu restants]",

	// M_cryptic
	"%zu / %zu dossiers, %zu / %zu fichiers, %s / %s, %zu / %zu erreurs",

	// M_cryptic_togo
	" - %zu restants",

	// M_err_fullpath
	"Erreur : chemin complet indisponible pour [%s].\n",

	// M_err_not_found
	"Erreur : chemin introuvable - [%s].\n",

	// M_err_generic
	"Erreur : %s\n",

	// M_path_label
	"Chemin : [%s]\n",

	// M_err_is_file
	"Erreur : le chemin désigne un fichier - [%s]\n",

	// M_confirm_folder
	"Supprimer [%s] et tout son contenu ?\n",

	// M_confirm_file
	"Supprimer le fichier [%s] ?\n",

	// M_confirm_prompt
	"Tapez o = oui / n = non : ",

	// M_cancelled
	"Annulé.\n",

	// M_completed_hint
	"Terminé en %s. Pour lister les erreurs : '--list-errors'.\n",

	// M_completed
	"Terminé en %s\n",

	// M_done_in
	" - terminé en %s\n",

	// M_errors_block
	"Erreurs :\n",

	// M_no_desc
	"<description indisponible>",

	// M_press_enter
	"Appuyez sur Entrée pour quitter...",

	// M_elev_refused
	"ATTENTION - élévation refusée, poursuite sans droits administrateur\n",

	// M_elev_hint
	"(les fichiers protégés par TrustedInstaller pourraient résister)\n",

	// M_unlocked
	"[unlock] processus %s (PID %lu) terminé\n",
};

//
const char * msg(MESSAGE id)
{
	__enforce(id > M_null && id < M_count);

	return (g_lang == LANG_FR) ? table_fr[id] : table_en[id];
}
