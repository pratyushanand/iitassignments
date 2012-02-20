#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string.h>

#define pr_info(args...) fprintf(stdout, ##args)
#define pr_err(args...) fprintf(stdout, ##args)
#define NOMEM "No memory available\n"
#define OPINVAL "Not a valid operation\n"
#define MAX_NAME_SIZE	50
#define MAX_PASSWORD_SIZE 20
#define MAX_AUTHOR_NAME_SIZE	50
#define MAX_BOOK_NAME_SIZE	50

#define USER_DATABASE "user_database"
#define BOOK_DATABASE "book_database"

enum bool {
	false,
	true,
};

enum rights {
	LIBRARIAN,
	MEMBER,
	GUEST,
};

enum menu {
	ADD_BOOK = 1,
	ADD_MEMBERS,
	CHANGE_PASSWORD,
	BORROW_BOOK,
	LOG_OUT,
	SEARCH_DATA_BASE,
	INVALID_MENU,
};

struct member_info {
	char name[MAX_NAME_SIZE];
	char password[MAX_PASSWORD_SIZE];
	enum rights right;
};
struct book_info {
	char author[MAX_AUTHOR_NAME_SIZE];
	char name[MAX_BOOK_NAME_SIZE];
	int copy;
};

void flush()
{
	char c;
	while((c = getchar()) != '\n' && c != EOF);
}

int login_success (char *name, struct member_info *info, FILE *user_db)
{
	struct member_info member;

	fseek(user_db, 0L, SEEK_SET);

	while (fread(&member, sizeof(member), 1, user_db) > 0) {
		if (!strcmp(name, &member.name[0])) {
			strcpy(info->name, &member.name[0]);
			strcpy(info->password, &member.password[0]);
			info->right = member.right;
			return true;
		}
	}
	return false;
}

void login (struct member_info *info, FILE *user_db)
{
	char name[MAX_NAME_SIZE];
	char password[MAX_PASSWORD_SIZE];
	int i;
	int login = false;
	char temp;

	do {
		i = 0;
		name[i] = '\0';
		pr_info("login:");
		do {
			name[i] = getchar();
			i++;
			if (i == MAX_NAME_SIZE) {
				pr_info("Too long user name\n");
				break;
			}
		} while (name[i-1] != '\n');
		name[i-1] = '\0';
		if (!strcmp(name, "guest")) {
			strcpy(info->name, "guest");
			info->right = GUEST;
			return;
		}
		login = login_success(name, info, user_db);
		if (!login)
			pr_info("Login does not exist\n");
		else {
			/* on successful login id match pasword */
			i = 0;
			pr_info("Password:");
			while (1) {
				/*
				 * does not tell user about max password
				 * lenght while login.
				 */
				temp = getchar();
				if (i < MAX_PASSWORD_SIZE) {
					password[i] = temp;
					i++;
				}
				if (temp == '\n')
					break;
			}
			password[i-1] = '\0';
			if (!strcmp(info->password, &password[0]))
				return;
			else {
				pr_info("Password does not match\n");
				login = false;
			}
		}
	} while (!login);
}

enum menu receive_menu(enum rights right)
{
	int menu;

	while (1) {
		switch(right) {
		case LIBRARIAN:
			pr_info("1. Add Book\n");
			pr_info("2. Add Members\n");
			pr_info("3. Change Password\n");
			pr_info("4. Borrow Book\n");
			pr_info("5. LOG_OUT\n");
			pr_info("6. SEARCH_DATA_BASE\n");
			pr_info("Enter your choice:");
			scanf("%d", &menu);
			if (menu >= ADD_BOOK &&
					menu <=	SEARCH_DATA_BASE)
				return (enum menu) menu;
		case MEMBER:
			pr_info("1. Change Password\n");
			pr_info("2. Borrow Book\n");
			pr_info("3. LOG_OUT\n");
			pr_info("4. SEARCH_DATA_BASE\n");
			pr_info("Enter your choice:");
			scanf("%d", &menu);
			menu += ADD_MEMBERS;
			if (menu >= CHANGE_PASSWORD &&
					menu <=	SEARCH_DATA_BASE)
				return (enum menu) menu;
		case GUEST:
			pr_info("1. LOG_OUT\n");
			pr_info("2. SEARCH_DATA_BASE\n");
			pr_info("Enter your choice:");
			scanf("%d", &menu);
			menu += BORROW_BOOK;
			if (menu >= LOG_OUT &&
					menu <=	SEARCH_DATA_BASE)
				return (enum menu) menu;
		}
	}
}

void add_user()
{
	struct member_info info;
	int i, rights;
	char temp;
	FILE *user_db;

	user_db = fopen(USER_DATABASE, "ab+");
	if (!user_db) {
		pr_info("Could not open user data base\n");
		return;
	}

	pr_info("Enter User Name:");
	flush();
	i = 0;
	do {
		info.name[i] = getchar();
		i++;
		if (i == MAX_NAME_SIZE) {
			pr_info("Too long user name\n");
			fclose(user_db);
			return;
		}
	} while (info.name[i-1] != '\n');
	if (i == 1) {
		pr_info("Too short user name\n");
		fclose(user_db);
		return;
	}
	info.name[i-1] = '\0';
	pr_info("Enter Password:");
	i = 0;
	while (1) {
		temp = getchar();
		if (i < MAX_PASSWORD_SIZE) {
			info.password[i] = temp;
			i++;
		} else {
			pr_info("Too long password\n");
			fclose(user_db);
			return;
		}
		if (temp == '\n')
			break;
	}
	info.password[i-1] = '\0';
	pr_info("Enter Rights: LIBRARIAN-0, Member 1:");
	scanf("%d", (int *)&rights);
	info.right = (enum rights) rights;
	if (info.right != LIBRARIAN && info.right != MEMBER) {
		pr_info("Wrong Rights\n");
		fclose(user_db);
		return;
	}
	if (fseek(user_db, 2, SEEK_END)) {
		pr_info("Problem with database\n");
		fclose(user_db);
		abort();
	}
	if (fwrite(&info, sizeof(info), 1, user_db) > 0)
		pr_info("Added successfully\n");
	fclose(user_db);
}

void add_book()
{
	struct book_info info;
	int i;
	FILE *book_db;

	book_db = fopen(BOOK_DATABASE, "ab+");
	if (!book_db) {
		pr_info("Could not open book data base\n");
		return;
	}

	pr_info("Enter Author Name:");
	flush();
	i = 0;
	do {
		info.author[i] = getchar();
		i++;
		if (i == MAX_AUTHOR_NAME_SIZE) {
			pr_info("Too long author name\n");
			fclose(book_db);
			return;
		}
	} while (info.author[i-1] != '\n');
	if (i == 1) {
		pr_info("Too short author name\n");
		fclose(book_db);
		return;
	}
	info.author[i-1] = '\0';
	pr_info("Enter Book Name:");
	i = 0;
	do {
		info.name[i] = getchar();
		i++;
		if (i == MAX_BOOK_NAME_SIZE) {
			pr_info("Too long book name\n");
			fclose(book_db);
			return;
		}
	} while (info.name[i-1] != '\n');
	if (i == 1) {
		pr_info("Too short book name\n");
		fclose(book_db);
		return;
	}
	info.name[i-1] = '\0';
	pr_info("Enter No of copies:");
	scanf("%d", &info.copy);
	if (fseek(book_db, 2, SEEK_END)) {
		pr_info("Problem with database\n");
		fclose(book_db);
		abort();
	}
	if (fwrite(&info, sizeof(info), 1, book_db) > 0)
		pr_info("Added successfully\n");
	fclose(book_db);
}

void search_data_base()
{
	int choice, i;
	char author[MAX_AUTHOR_NAME_SIZE];
	char name[MAX_BOOK_NAME_SIZE];
	struct book_info info;
	FILE *book_db;

	book_db = fopen(BOOK_DATABASE, "rb");
	if (!book_db) {
		pr_info("Could not open book data base\n");
		return;
	}
	pr_info("1. Author Search:");
	pr_info("2. Book Search:");
	scanf("%d", &choice);
	i = 0;
	flush();
	if (choice == 1) {
		pr_info("Enter name of author\n");
		do {
			author[i] = getchar();
			i++;
			if (i == MAX_AUTHOR_NAME_SIZE) {
				pr_info("Too long author name\n");
				fclose(book_db);
				return;
			}
		} while (author[i-1] != '\n');
		author[i-1] = '\0';
		while (fread(&info, sizeof(info), 1, book_db) > 0) {
			if (!strcmp(author, &info.author[0])) {
				pr_info("Book found in database\n");
				if (info.copy == 0)
					pr_info("But Sorry, All book are issued\n");
				else
					pr_info("%d books are present\n", info.copy);
				fclose(book_db);
				return;
			}
		}
	} else {
		pr_info("Enter name of book\n");
		do {
			name[i] = getchar();
			i++;
			if (i == MAX_BOOK_NAME_SIZE) {
				pr_info("Too long book name\n");
				fclose(book_db);
				return;
			}
		} while (name[i-1] != '\n');
		name[i-1] = '\0';
		while (fread(&info, sizeof(info), 1, book_db) > 0) {
			if (!strcmp(name, &info.name[0])) {
				pr_info("Book found in database\n");
				if (info.copy == 0)
					pr_info("But Sorry, All book are issued\n");
				else
					pr_info("%d books are present\n", info.copy);
				fclose(book_db);
				return;
			}
		}

	}
	pr_info("Book is not in data base\n");
	fclose(book_db);
}

void borrow_book()
{
	char name[MAX_BOOK_NAME_SIZE];
	struct book_info info;
	int i, copy, count;
	FILE *book_db;

	book_db = fopen(BOOK_DATABASE, "rb+");
	if (!book_db) {
		pr_info("Could not open book data base\n");
		return;
	}

	pr_info("Enter Name of book to be borrowed:");
	flush();
	i = 0;
	do {
		name[i] = getchar();
		i++;
		if (i == MAX_BOOK_NAME_SIZE) {
			pr_info("Too long book name\n");
			fclose(book_db);
			return;
		}
	} while (name[i-1] != '\n');
	name[i-1] = '\0';
	count = 0;
	while (fread(&info, sizeof(info), 1, book_db) > 0) {
		if (!strcmp(name, &info.name[0])) {
			if (info.copy == 0) {
				pr_info("But Sorry, All books are issued\n");
				fclose(book_db);
				return;
			}

			fclose(book_db);
			book_db = fopen(BOOK_DATABASE, "rb+");
			fseek(book_db, count * sizeof(info) +
					sizeof(info.author) +
					sizeof(info.name), SEEK_SET);
			copy = info.copy - 1;
			fwrite(&copy, sizeof(int), 1, book_db);
			fclose(book_db);
			pr_info("Book issued\n");
			return;
		}
		count++;
	}
	pr_info("Book not in database\n");
	fclose(book_db);
}

void execute_menu(enum menu menu)
{

		switch (menu) {
		case LOG_OUT:
			break;
		case ADD_MEMBERS:
			add_user();
			break;
		case ADD_BOOK:
			add_book();
			break;
		case CHANGE_PASSWORD:
			pr_info("Not Supported\n");
			break;
		case BORROW_BOOK:
			borrow_book();
			break;
		case SEARCH_DATA_BASE:
			search_data_base();
			break;
	}
}

/*
 * main routine.
 * receives input from user and calls subroutine accordingly.
 */

int main(int argc, char** argv)
{
	struct member_info info;
	FILE *user_db, *book_db;
	int ret;
	enum menu menu = INVALID_MENU;

	user_db = fopen(USER_DATABASE, "ab+");
	if (!user_db) {
		pr_info("Could not open user data base\n");
		return -EINVAL;
	}
	if (fread(&info, sizeof(info), 1, user_db) < 1) {
		/* add default admin suer */
		strcpy(&info.name[0], "admin");
		strcpy(&info.password[0], "admin");
		info.right = LIBRARIAN;
		fwrite(&info, sizeof(info), 1, user_db);
	}

	login(&info, user_db);
	fclose(user_db);
	do {
		menu = receive_menu(info.right);
		execute_menu(menu);
	} while (menu != LOG_OUT);

	return 0;
}
