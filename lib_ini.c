/**
 *  @file lib_ini.c
 *  @brief Gestion des fichiers de type paramétrage windows .ini
 *  @author Beaulande Laurent
 *  @date   Juin 2005
 *  
 * Fonctions permettant de gérér un fichier de configuration de type windows .ini\n
 * [section]\n
 * param=valeur\n
 * 
 * Possibilité de mettre en commentaire une ligne par le caractère '#'\n
 * La liste est géré à la manière d'une liste chainée
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "lib_ini.h"

typedef struct apr_pool_t {
	void **ptrTab;
	int size_max;
	int size;
} apr_pool_t;

/**
 * Définie un paramètre avec sa valeur et pointe sur le paramètre suivant\n
 * si c'est le dernier, la valeur de 'pstParamSuivante' vaut NULL
 */
typedef struct stParamsValues{
	char *szParam; ///< Nom du paramètre
	char *szValue; ///< Valeur du paramètre
	pstParamsValues pstParamSuivante; ///< structure de paramètre/Valeur suivante
} stParamsValues;


/**
 * Définie une section et pointe sur la section suivante\n
 * si c'est le dernier, la valeur de 'pstSectionSuivante' vaut NULL
 */
typedef struct UneSection{
	char *szSection; ///< Nom de la section
	short iNbParam;	 ///< nombre de paramètre
	pstParamsValues pstTabParamsValues; ///< Liste des paramètre, pointe sur le premier paramètre
	pstParamsValues pstTabParamsValuesDernier; ///< Liste des paramètre, pointe sur le dernier
	pstSection pstSectionSuivante; ///< Section suivante
} stSection;


static apr_pool_t * p;
static stIni * ctxIni;

static short extraireParamsValeurs(apr_pool_t*p,char *szBuffer);
static short ajouterUneSection(apr_pool_t*p,char *szSection);
static short ajouterUnParamValue(apr_pool_t*p,char *szBuffer);
static short estCommentaire(char *buffer);
static char *deleteSpace(char *buffer);
static char *rechercherParamValue(char *param,pstSection Section);

apr_pool_t * apr_memory_create()
{
	apr_pool_t * tab = (apr_pool_t*) malloc(sizeof(apr_pool_t));
	tab->ptrTab = (void*) malloc(sizeof(void) * 1024 );
	tab->size_max = 1024;
	tab->size = 0;
	memset(tab->ptrTab,0,tab->size_max);
	return tab;
}

void apr_memory_destroy(apr_pool_t * pool)
{
	int i;
	for(i=pool->size-1;i<0;i--) {
		if ( pool->ptrTab[i] != NULL )
			free(pool->ptrTab[i]);
	}
	
	free(pool->ptrTab);
	free(pool);
}

void apr_memory_free(apr_pool_t * pool)
{
	int i=0;
	for(i=pool->size-1;i<0;i--) {
		if ( pool->ptrTab[i] != NULL )
			free(pool->ptrTab[i]);
			pool->ptrTab[i] = NULL;
	}
	pool->size=0;
}

void * apr_palloc(apr_pool_t * p,size_t size)
{
	if ( p->size == p->size_max )
		return NULL;
	
	void * ptr = malloc(size);
	p->ptrTab[p->size] = ptr;
	p->size++;
	return ptr;
}

char * 	apr_pstrdup (apr_pool_t *p, const char *s)
{
	char * new_str = (char*) apr_palloc(p,sizeof(char) * (strlen(s) + 1 ));
	if ( new_str == NULL ) return NULL;
	memset(new_str,'\0',strlen(s) + 1);
	strcpy(new_str,s);
	
	return new_str;
}

/**
 *  @brief Charge un fichier en mémoire
 *  @param[in] szNomFichier - Répertoire et Nom du fichier de configuration
 *  @return INI_SUCCESS si OK
 *  @return INI_OPEN_FAILED si Impossible d'ouvrir le fichier
 *  @return INI_PROBLEM_MEMORY erreur d'allocation memoire
 * 
 *  Charge en mémoire un fichier de configuration
 */
short ChargerFichierIni(char *szNomFichier)
{
	p = apr_memory_create();
	ctxIni=NULL;
	short iRetour=INI_SUCCESS;
	FILE * fDesc;
	char szBuffer[INI_BUFFER_LENGTH];
	
	fDesc = fopen(szNomFichier,"r");
	if ( fDesc == NULL ) {
		iRetour = INI_OPEN_FAILED;
	}
	else {
		
		ctxIni = (stIni *)apr_palloc(p,sizeof(stIni));
		if ( ctxIni == NULL ) {
			return INI_PROBLEM_MEMORY;
		}
		
		ctxIni->iNbSection = 0;
		ctxIni->pstSections = NULL;
		ctxIni->pstSectionsDerniere = NULL;

		// creation d'une section sans nom pour les parametres qui ne sont dans 
		// aucune section
		pstSection sectionNULL = (pstSection)apr_palloc(p,sizeof(stSection));
		if ( sectionNULL == NULL ) {
			return INI_PROBLEM_MEMORY;
		}
		sectionNULL->szSection = NULL;
		sectionNULL->pstSectionSuivante = NULL;
		sectionNULL->iNbParam = 0;
		sectionNULL->pstTabParamsValues = NULL;
		sectionNULL->pstTabParamsValuesDernier = NULL;
	
		ctxIni->pstSections = sectionNULL;
		ctxIni->pstSectionsDerniere = sectionNULL;
		ctxIni->iNbSection++;

		while(fgets(szBuffer,INI_BUFFER_LENGTH,fDesc) != NULL) {
			if ( extraireParamsValeurs(p,szBuffer) == -1 ) {
				iRetour = -2;
				fclose(fDesc);
				break;
			}
			memset(szBuffer,'\0',INI_BUFFER_LENGTH);
		}
		fclose(fDesc);
	}
	return iRetour;	
}

void ReleaseIni() {
	apr_memory_destroy(p);
}


/**
 *  @brief Recherche dans une chaine de caractère les données d'un fichier de configuration
 *  @param[in] p : pool memoire
 *  @param[in] ctxIni : liste des parametres
 *  @param[in] szBuffer - une Ligne du fichier de configuration
 *  @return INI_SUCCESS :OK
 *  @return INI_PROBLEM_MEMORY : erreur d'allocation mémoire
 *
 *  Recherche dans une ligne recupérée d'un fichier de configuration s'il y a une déclaration
 *  de section ou la définition d'un paramètre.\n
 *  Si la ligne commence ou comporte un '#', elle n'est pas prise en compte et la fonction
 *  retourne INI_SUCCESS car ce n'est pas considéré comme une erreur.
 */
static short extraireParamsValeurs(apr_pool_t*p,char *szBuffer)
{
	short iRetour = INI_SUCCESS;
	char *pPositionDebut,*pPositionFin;
	
	// pas de commentaire
	if ( estCommentaire(szBuffer) == 0 ) {

		if ( strchr(szBuffer,CARAC_PARAM_VALUE) != NULL ) {
			// c'est une ligne PARAM=VALUE
			iRetour = ajouterUnParamValue(p,szBuffer);
		}
		else {
			// recherche si c'est une section
			pPositionDebut = strchr(szBuffer,CARAC_DEBUT_SECTION);
			if ( pPositionDebut != NULL ) {
				pPositionFin = strchr(szBuffer,CARAC_FIN_SECTION);
				if ( pPositionFin != NULL && pPositionDebut < pPositionFin ) {
					*pPositionFin = '\0';
					iRetour = ajouterUneSection(p,pPositionDebut+1);
				}
			}
		}
	}
	return iRetour;
}

/**
 *  @brief verifie si une ligne est en commentaire
 *  @param[in] buffer : ligne
 *  @return 1 : la ligne est un commentaire
 *  @return 0 : pas de commentaire
 *
 *   verifie si le caractere de commentaire se trouve en debut de ligne
 *   ce caractere peut etre precede par des espaces ou des tabulations
 */
static short estCommentaire(char *buffer)
{
	int i;
	for(i=0;i<strlen(buffer) && (buffer[i] == ' ' || buffer[i]=='\t');i++);
	
	if ( i<strlen(buffer) ) {
		if ( buffer[i] == CARAC_COMMENTAIRE ) {
			return 1;
		}
	}
	return 0;
}

/**
 *  @brief Ajoute un paramètre et sa valeur dans la liste
 *  @param[in] p - pool memoire
 *  @param[in] ctxIni - liste des parametres
 *  @param[in] szBuffer - ligne brute param=valeur
 *  @return INI_SUCCESS :OK
 *  @return INI_PROBLEM_MEMORY : erreur d'allocation mémoire
 *
 *  Ajoute un paramètre et sa valeur à la fin de la liste chainée des paramètres.\n
 *  szBuffer est de type param=value.\n
 *  D'abord on remplace les caractères parasites comme le retour chariot ou saut de ligne par le caractère fin de chaine '\\0'\n
 *  On sépare le nom du paramètre de sa valeur et on initialise la structure 'stParamsValues'\n
 *  Enfin s'il y a déjà des paramètres on l'ajoute à la fin la liste.
 */
static short ajouterUnParamValue(apr_pool_t*p,char *szBuffer)
{
	char *szParam,*szValue;
	pstParamsValues pstTmp;
	
	// suppression des caracteres parasites de fin de ligne
	szBuffer = deleteSpace(szBuffer);

	// recherche du premier =
	szValue = strchr(szBuffer,CARAC_PARAM_VALUE);
	*szValue = '\0';
	szValue++;
	szParam = szBuffer;

	pstTmp = (pstParamsValues) apr_palloc(p,sizeof(stParamsValues));
	if ( pstTmp == NULL )
		return INI_PROBLEM_MEMORY;
	
	pstTmp->szParam = apr_pstrdup(p,szParam);
	
	if ( szValue == NULL ) {
		pstTmp->szValue = NULL;
	}
	else {
		pstTmp->szValue = apr_pstrdup(p,szValue);
	}

	pstTmp->pstParamSuivante = NULL;

	if ( ctxIni->pstSectionsDerniere->pstTabParamsValuesDernier == NULL )
		ctxIni->pstSectionsDerniere->pstTabParamsValues = pstTmp;
	else
		ctxIni->pstSectionsDerniere->pstTabParamsValuesDernier->pstParamSuivante = pstTmp;

	ctxIni->pstSectionsDerniere->pstTabParamsValuesDernier = pstTmp;
	ctxIni->pstSectionsDerniere->iNbParam++;
	return INI_SUCCESS;
}

/**
 *  @brief Supprime les caracteres espace, fin de ligne, retour chariot ...
 *  a la fin de la ligne et au debut
 *  @param[in] buffer
 *  @return char * : chaine sans les caracteres
 *
 */
static char *deleteSpace(char *buffer)
{
	int i;
	for(i=strlen(buffer)-1;i>=0;i--) {
		if ( isspace(buffer[i]) )
			buffer[i] = '\0';
		else {
			break;
		}
	}
	
	for(i=0;i<strlen(buffer);i++) {
		if ( ! isspace(buffer[i]) )
			break;
	}
	
	return buffer+i;
}

/**
 *  @brief Ajoute une section dans la liste chainée
 *  @param[in] p : pool memoire
 *  @param[in] ctxIni : contexte contenant la liste chaine
 *  @param[in] szSection - nom de la section

 *  @return INI_SUCCESS si OK
 *  @return INI_PROBLEM_MEMORY si erreur d'allocation mémoire
 *
 */
static short ajouterUneSection(apr_pool_t*p,char *szSection)
{
	pstSection pstTmp = (stSection *) apr_palloc(p,sizeof(stSection));
	if ( pstTmp == NULL ) {
		return INI_PROBLEM_MEMORY;
	}

	pstTmp->pstSectionSuivante = NULL;
	pstTmp->szSection = apr_pstrdup(p,szSection);
	pstTmp->pstTabParamsValues = NULL;
	pstTmp->pstTabParamsValuesDernier = NULL;
  pstTmp->iNbParam = 0;

	if ( ctxIni->pstSections == NULL ) {
			ctxIni->pstSections = pstTmp;
	}
	else {
		ctxIni->pstSectionsDerniere->pstSectionSuivante = pstTmp;
	}

	ctxIni->pstSectionsDerniere = pstTmp;
	ctxIni->iNbSection++;

	return INI_SUCCESS;
}

/**
 *  @brief Recherche la valeur d'un paramètre
 *  @param[in] ctxIni - liste des parametres
 *  @param[in] szSection - Nom de la section
 *  @param[in] szParam - Nom du paramètre
 *  @return valeur du parametre
 *  @return NULL si pas trouve ou pas de valeur
 *
 *  Recherche la valeur d'un paramètre dans une section.\n
 *  Parcour d'une liste chainée
 */
char *rechercherValeur(char *szSection,char *szParam)
{
	pstSection pSectionCourante; 
	char *szValueTmp=NULL;
	
	if ( ctxIni == NULL )
		return NULL;

	if ( ctxIni->pstSections == NULL ) {
		return NULL;
	}
	else {
		pSectionCourante = ctxIni->pstSections;
		while ( pSectionCourante != NULL ) {
			if ( pSectionCourante->szSection == NULL ) {
				if ( szSection == NULL )
					szValueTmp = rechercherParamValue(szParam,pSectionCourante);
			}
			else {
				if ( strcmp(pSectionCourante->szSection,szSection) == 0 ) {
					szValueTmp = rechercherParamValue(szParam,pSectionCourante);
				}
			}
			
			if ( szValueTmp != NULL ) {
				return szValueTmp;
			}
			pSectionCourante = pSectionCourante->pstSectionSuivante;
		}
	}
	return NULL;
}

/**
 *  @brief Recherche la valeur d'un paramètre dans une section
 *  @param[in] param - Nom du paramètre
 *  @param[in] Section - pointeur sur la section
 *  @return char * - valeur du paramètre
 *  @return NULL si pas trouvé
 *
 */
static char *rechercherParamValue(char *param,pstSection Section)
{
	char *pRetour=NULL;
	pstParamsValues pstParamTmp;
	
	pstParamTmp = Section->pstTabParamsValues;
	while (pstParamTmp != NULL ) {
		if ( strcmp(pstParamTmp->szParam,param) == 0 ) {
			pRetour = pstParamTmp->szValue;
			break;
		}
		pstParamTmp = pstParamTmp->pstParamSuivante;
	}

	return pRetour;
}

/**
 *  @brief donne le nombre de section en memoire
 *  @param[in] ctxIni - liste des parametres
 *  @return nombre de section
 *
 */
int nbrSection()
{
	return ctxIni->iNbSection;
}

/**
 *  @brief donne le nombre de parametre en memoire pour une section
 *  @param[in] ctxIni - liste des parametres
 *  @param[in] szSection - nom de la section
 *  @return nombre de parmatres
 *
 */
int nbrParametres(char *szSection)
{
	pstSection pSectionCourante; 
	
	if ( ctxIni->pstSections == NULL ) {
		return INI_IS_EMPTY;
	}
	else {
		pSectionCourante = ctxIni->pstSections;
		while ( pSectionCourante != NULL ) {
			if ( pSectionCourante->szSection == NULL ) {
				if ( szSection == NULL )
					return pSectionCourante->iNbParam;
			}
			else {
				if ( strcmp(pSectionCourante->szSection,szSection) == 0 ) {
					return pSectionCourante->iNbParam;
				}
			}
			pSectionCourante = pSectionCourante->pstSectionSuivante;
		}
	}
	return INI_SECTION_NOT_FOUND;
}
/**
 *  @brief Liste tous les paramètres d'une section
 *  @param[in] ctxIni - liste des parametres
 *  @param[in] szSection - Nom de la section
 *  @return stTabParam* - enregistrement des noms de paramètre
 *  @return NULL si pas de données ou pas trouvé ou pas de paramètre
 *
 *  Récupère la liste de tous les paramètres d'une section.\n
 *  On parcoure la liste chainée des sections jusqu'a la section désirée.\n
 *  Puis on parcoure la liste chainée des paramètres en incrémentant un compteur
 *  et sauvegarde dans un tableau de pointeur, le nom du paramètre.
 */
stTabParam* TrouverParametres(char *szSection)
{
	stTabParam* stRetour = NULL;
	pstSection pstTmp; 
	pstParamsValues pstParamTmp;
	
	if ( ctxIni->pstSections == NULL ) {
		stRetour = NULL;
	}
	else {
		pstTmp = ctxIni->pstSections;
		while ( pstTmp != NULL ) {
			int bonneSection = 0;
			// recherche la liste des parametres dans la section sans balise
			if ( pstTmp->szSection == NULL ) {
				// on a trouve la bonne section puisque le client desire avoir 
				// les parametres sans balise section=NULL
				if ( szSection == NULL ) {
					// il n'y a pas de parametre sans balise
					if ( pstTmp->iNbParam == 0 )
						return NULL;
					bonneSection=1;
				}
			}
			else {
				if ( strcmp(pstTmp->szSection,szSection) == 0 ) {
					if ( pstTmp->iNbParam == 0 )
						return NULL;
					bonneSection=1;
				}
			}
			
			if ( bonneSection == 1 ) {
				stRetour = (stTabParam *)apr_palloc(p,sizeof(stTabParam));
				if ( stRetour == NULL )
					return NULL;
				
				stRetour->pTabParam = ( char **) apr_palloc(p,sizeof(char *)*pstTmp->iNbParam);
				if ( stRetour->pTabParam == NULL )
					return NULL;
				stRetour->iNbParametre = 0;
				
				pstParamTmp = pstTmp->pstTabParamsValues;
				while (pstParamTmp != NULL ) {
					stRetour->pTabParam[stRetour->iNbParametre] = pstParamTmp->szParam;
					pstParamTmp = pstParamTmp->pstParamSuivante;
					stRetour->iNbParametre++;
				}
				break;
			}
			pstTmp = pstTmp->pstSectionSuivante;
		}
	}
	return stRetour;		
}

/**
 *  @brief Affiche les données de la liste
 *  @return Rien
 *
 *  Affiche à l'écran, les données se trouvant en mémoire d'un fichier de configuration.\n
 *  Utile surtout pour les tests d'un programme et lors du developpement de ces fonctions.
 */
void Dump()
{
	pstSection sec;
	pstParamsValues param;
	
	printf("----------------Dump Fichier ini----------------\n\n");
	printf("--- Nombre de section : %d\n",ctxIni->iNbSection);
	
	sec = ctxIni->pstSections;
	while(sec != NULL) {
		printf("\n---- Section [%s] ---- \n",sec->szSection);	
		printf("---- Nombre de parametre [%d] ---- \n",sec->iNbParam);	
		param = sec->pstTabParamsValues;
		while ( param != NULL ) {
			printf("-Param [%s] - Value [%s]\n",param->szParam,param->szValue);
			param = param->pstParamSuivante;
		}
		sec = sec->pstSectionSuivante;
	}
	printf("\n----------------Dump Fichier ini----------------\n");
}
