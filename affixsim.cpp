/*
*	Affix Reroll Simulator
*	by biribiri#3681
* 
*	The purpose of this program is to run a simulation on how long it will take to get a certain value of an affix
*	This code will run 10,000 iterations over the code until we get the required value, as well as calculate the average
*	amount of materials consumed at any point.
* 
*	For reference, the standard ranges for the following values are below:
* 
*	Tier 2 Affixes:
*	General Attack: 7 to 13 ATK
*	Specific Attack: 8.4 to 15.6 ATK
*	Critical Damage: 2.10% to 3.80% CDMG
*	SP/s: 0.18 to 0.34 SP/s
*	SP% Cost Reduction: 1.20% to 2.20% SP
* 
*	Tier 3 Affixes:
*	General Attack: 12 to 19 ATK
*	Specific Attack: 14.5 to 23 ATK
*	Critical Damage: 3.40% to 5.60% CDMG
*	SP/s: 0.30 to 0.50 SP/s
*	SP% Cost Reduction: 2.00% to 3.20% SP
* 
*	*Note that the values use real affix data sampled from multiple accounts; hidden decimals may slightly affect the ranges
* 
*	The types of affixes are listed below:
*	Weapon ATK		Type ATK	generic stats	per sec		added dmg	Resistence	Reductions	Additions
*	Great Sword/Fist	BIO		ATK		SP/s		Ice		Ice		-SP%		CDMG%
*	Heavy/Scythe		PSY		DEF		HP/s		Fire		Fire
*	Blade/Cross		MECH		CRT				Lightining	Lightning
*	Lance/Pistol		QUA		SP				Max Shield
*	Archers/Chakram		IMG		HP
* 
*	Credits for above data: Clicky#3131, L e o#1173, SinsOfSeven#3164 (TencentDimepiece), Zeikire#9999
*
*	To simulate affix rolling, there are 3 steps that take place:
* 	1) Type of affix. We select 1 affix type from the above choices.
*	2) Grade of affix. 5* stimgata can have a Tier 2 or Tier 3 grade; basically, a 50/50.
*	3) Value of affix. Using the discovered ranges, we pick a random value from the range. This assume no weighting on the affix
*	i.e. all values are equally likely to occur.
* 
*	We run this to select 2 affixes. If there's a value that's specified to be locked, we instead only roll 1 affix after 1 of
*	2 affixes reaches the locked value.
*/

#include <stdlib.h>
#include <time.h>
#include <iostream>
#include <algorithm>

using namespace std;

int run = 0;
const int iter = 10000; // iterations for averaging purposes

int prompt1 = -1, prompt2 = -1;
int PROMPT_MAX = 4;
int AFFIX_OPEN[] = { 0, 1, 2, 3 };
string AFFIX_TYPE[] = { "ATK", "CDMG", "SP/s", "SP%" };
double AFFIX_MIN[] = {7, 2.10, 0.18, 1.20};
double AFFIX_MAX[] = {23, 5.60, 0.50, 3.20};

int count[4] = { 0 };

bool WAFER_ON;

const double affix_reg = 40;
const double affix_waf = 300;
const double wafer = 20;

double affix_data[26][2][2] = {{{0}}};
/*
*	For this array, here are the column representations:
*	Column 1: Type of affix (HP, Heavy+Scythe, etc.)
*	Column 2 Grade of affix (T2/T3)
*	Column 3: Min/Max (0 = min/1 = max)
*	Since each battlesuit has 2 Specific affixes (Type, Weap) we will add the value for it twice to the data.
*/

double ncc_total = 0;
double wafer_total = 0;

double goals_affix[4] = { 0 };
double goals_wafer[4] = { 0 };

/*
*	Postcondition:	Fills the affix ranges.
*/
void fill_data() {
	// Tier 2 affixes
	affix_data[0][0][0] = 7;
	affix_data[0][0][1] = 13;
	affix_data[1][0][0] = 8.4;
	affix_data[1][0][1] = 15.6;
	affix_data[2][0][0] = 8.4;
	affix_data[2][0][1] = 15.6;
	affix_data[3][0][0] = 2.10;
	affix_data[3][0][1] = 3.80;
	affix_data[4][0][0] = 0.18;
	affix_data[4][0][1] = 0.34;
	affix_data[5][0][0] = 1.20;
	affix_data[5][0][1] = 2.20;

	// Tier 3 affixes
	affix_data[0][1][0] = 12;
	affix_data[0][1][1] = 19;
	affix_data[1][1][0] = 14.5;
	affix_data[1][1][1] = 23;
	affix_data[2][1][0] = 14.5;
	affix_data[2][1][1] = 23;
	affix_data[3][1][0] = 3.40;
	affix_data[3][1][1] = 5.60;
	affix_data[4][1][0] = 0.30;
	affix_data[4][1][1] = 0.50;
	affix_data[5][1][0] = 2.00;
	affix_data[5][1][1] = 3.20;
}

/*
*	Postcondition:	Prints the list of desirable affixes.
*/
void prompt_affix() {
	for (int i = 0; i < PROMPT_MAX; i++) {
		cout << i << ":\t";
		switch (AFFIX_OPEN[i]) {
		case 0:
			cout << "Attack\t\t\t\t (Gain X ATK / X-type characters gain Y ATK / X and Y Wielders gain Z ATK)" << endl;
			break;
		case 1:
			cout << "Critical Damage\t\t\t (Increase Crit DMG X%)" << endl;
			break;
		case 2:
			cout << "SP per second\t\t\t (Regain X SP per second when enemies are near)" << endl;
			break;
		case 3:
			cout << "SP Cost Reduction\t\t (Reduce skill SP cost by X%)" << endl;
			break;
		}
	}
}

/*
*	Postcondition:	Returns whether the input is between or equal to the range defined.
*/
bool valid_prompt(double prompt, double max, double min = 0) {
	return prompt >= min && prompt <= max;
}

/*
*	Postcondition:	Saves the user's selected prompt.
*/
void select_desired(int& prompt) {
	prompt_affix();
	while (!valid_prompt(prompt, PROMPT_MAX-1)) {
		cin >> prompt;
		if (cin.fail() || !valid_prompt(prompt, PROMPT_MAX-1)) {
			cin.clear();
			cin.ignore(numeric_limits<streamsize>::max(), '\n');
			prompt = -1;
			cout << "Please enter a valid number between 0 and " << PROMPT_MAX-1 << endl;
		}
	}
	prompt = AFFIX_OPEN[prompt];
}

/*
*	Posctondition:	Determines whether the user wants to use Wafer Locks.
*/
void select_wafer() {
	int yn = -1;
	cout << "Enable Wafer Stabilizers?" << endl;
	cout << "0:\tNo" << endl;
	cout << "1:\tYes" << endl;
	while (!valid_prompt(yn, 1)) {
		cin >> yn;
		if (cin.fail() || !valid_prompt(yn, 1)) {
			cin.clear();
			cin.ignore(numeric_limits<streamsize>::max(), '\n');
			yn = -1;
			cout << "Please choose 1 or 0" << endl;
		}
	}
	WAFER_ON = (yn == 1) ? true : false;
}
/*
*	Postcondition:	Determines the user's targets for the end affix result.
*/
void set_goals() {
	for (int i = 0; i < 4; i++) {
		int c = ::count[i];
		double min = AFFIX_MIN[i] * c;
		double max = AFFIX_MAX[i] * c;
		if (c > 0) {
			cout << "Set goal for " << AFFIX_TYPE[i] << " affix in range " << min << " to " << max << ": " << endl;
			double goal = -1;
			while (!valid_prompt(goal, max, min)) {
				cin >> goal;
				if (cin.fail() || !valid_prompt(goal, max, min)) {
					cin.clear();
					cin.ignore(numeric_limits<streamsize>::max(), '\n');
					goal = -1;
					cout << "Please enter a valid number from " << min << " to " << max << endl;
				}
			}
			goals_affix[i] = goal;
		}
	}
}
/*
*	Postcondition:	Gets a random affix.
*/
double get_affix() {
	int type = rand() % 26;
	int tier = rand() % 2;
	double f = (double)rand() / RAND_MAX;
	double affix = affix_data[type][tier][0] + f * (affix_data[type][tier][1] - affix_data[type][tier][0]);

	// Since the affix values are now "diluted," we add an encoding via the original typing to help us distinguish which affixes it came from to help us group.
	return affix + type * 100;
}
/*
*	Postcondition:	Adds the affix to the current array.
*/
void add_stat(double arr[], double affix) {
	int head = (int)(affix / 100);
	double tail = affix - head * 100;

	if (valid_prompt(head, 2)) {
		arr[0] += tail;
	}
	else if (head == 3) {
		arr[1] += tail;
	}
	else if (head == 4) {
		arr[2] += tail;
	}
	else if (head == 5) {
		arr[3] += tail;
	}
}
/*
*	Postcondition:	Determines wafer goals.
*/
void set_wafer() {
	for (int i = 0; i < 4; i++) {
		int c = ::count[i];
		double target = goals_affix[i];
		double min = AFFIX_MIN[i];
		double max = AFFIX_MAX[i];

		double wafer_min = std::max(min, target - max);
		double wafer_max = std::min(max, target);

		if (c == 1) {
			cout << "Set goal for " << AFFIX_TYPE[i] << " affix in range " << goals_affix[i] << " to " << max << ": " << endl;
			double goal = -1;
			while (!valid_prompt(goal, max, goals_affix[i])) {
				cin >> goal;
				if (cin.fail() || !valid_prompt(goal, max, goals_affix[i])) {
					cin.clear();
					cin.ignore(numeric_limits<streamsize>::max(), '\n');
					goal = -1;
					cout << "Please enter a valid number from " << goals_affix[i] << " to " << max  << endl;
				}
			}
			goals_wafer[i] = goal;
		}
		else if (c == 2) {
			cout << "Set goal for " << AFFIX_TYPE[i] << " affix in range " << wafer_min << " to " << wafer_max << ": " << endl;
			double goal = -1;
			while (!valid_prompt(goal, wafer_max, wafer_min)) {
				cin >> goal;
				if (cin.fail() || !valid_prompt(goal, wafer_max, wafer_min)) {
					cin.clear();
					cin.ignore(numeric_limits<streamsize>::max(), '\n');
					goal = -1;
					cout << "Please enter a valid number from " << wafer_min << " to " << wafer_max << endl;
				}
			}
			goals_wafer[i] = goal;
		}
	}
}
/*
*	Postcondition:	Failsafe measure to ensure affix values are within range.
*/
void check_wafer() {
	bool valid_target = false;
	while (!valid_target) {
		valid_target = true;
		cout << "Set Wafer Stabilizer targets" << endl;
		set_wafer();

		for (int i = 0; i < 4; i++) {
			if (goals_affix[i] != 0) {
				/*
				*	2 conditions:
				*	1)	Single affix
				*		Make sure lock target is greater than or equal to affix target
				*	2)	Double affix
				*		Make sure lock target is within -23 of affix target
				*/
				switch (::count[i]) {
				case 0:
					break;
				case 1:
					if (goals_wafer[i] < goals_affix[i]) {
						valid_target = false;
						cout << "Invalid value; lock target must be greater than or equal to affix target" << endl;
					}
					break;
				case 2:
					if (goals_affix[i] - goals_wafer[i] > 23) {
						valid_target = false;
						cout << "Invalid value; lock target is too far from affix target" << endl;
					}
					break;
				}
			}
		}
	}
	cout << "Target set" << endl;
}
/*
*	Postcondition:	Checks and rerolls affix to guarantee no double SP rolls on the same 2x affix roll.
*/
void sp_check(double affix1, double& affix2) {
	double head1 = (int) (affix1 / 100);
	double head2 = (int) (affix2 / 100);
    	if ((head1 == 4 || head1 == 5) && (head1 == head2)) {
		while (head1 == head2) {
			affix2 = get_affix();
			head2 = (int) (affix2 / 100);
		}
	}
}
/*
*	Postcondition:	Checks whether the head matches the affix.
*/
bool head_check(double head, int affix) {
	switch (affix) {
	case 0:
		return head == 0 || head == 1 || head == 2;
	case 1:
		return head == 3;
	case 2:
		return head == 4;
	case 3:
		return head == 5;
	}
}
/*
*	Postcondition:	Simulates affixing without using wafers.
*/
void sim_reg() {
	double affix1, affix2 = 0;
	bool done = false;
	int rep = 0;
	while (!done) {
		++rep;
		affix1 = get_affix();
		affix2 = get_affix();
		sp_check(affix1, affix2);

		double curr[4] = { 0 };
		add_stat(curr, affix1);
		add_stat(curr, affix2);
		
		done = true;
		for (int i = 0; i < 4; i++) {
			if (curr[i] < goals_affix[i]) {
				done = false;
				break;
			}
		}
	}
	ncc_total += rep * affix_reg;
}
/*
*	Precondition:	Simulates affixing with wafers.
*/
void sim_wafer() {
	double affix1, affix2 = 0;
	double affix_locked = 0;
	double save[4] = { 0 };
	for (int i = 0; i < 4; i++) {
		save[i] = goals_affix[i];
	}

	bool locked = false;
	bool done = false;
	int rep = 0;

	while (!locked) {
		++rep;
		affix1 = get_affix();
		affix2 = get_affix();
		sp_check(affix1, affix2);

		double curr[4] = { 0 };
		add_stat(curr, affix1);
		add_stat(curr, affix2);

		done = true;
		for (int i = 0; i < 4; i++) {
			if (curr[i] < goals_affix[i]) {
				done = false;
				break;
			}
		}
		if (done) {
			ncc_total += rep * affix_reg;
			return;
		}

		double head1 = (int) (affix1 / 100);
		double head2 = (int) (affix2 / 100);
		double tail1 = affix1 - head1 * 100;
		double tail2 = affix2 - head2 * 100;

		for (int i = 0; i < 4; i++) {
			double compare1 = 0;
			double compare2 = 0;
			if (::count[i] == 1) {
				if ((tail1 >= goals_affix[i] && head_check(head1, i)) || (tail2 >= goals_affix[i] && head_check(head2, i))) {
					switch (i) {
					case 0:
						if (head1 == 0 || head1 == 1 || head1 == 2) compare1 = tail1;
						if (head2 == 0 || head2 == 1 || head2 == 2) compare2 = tail2;
						break;
					case 1:
						if (head1 == 3) compare1 = tail1;
						if (head2 == 3) compare2 = tail2;
						break;

					case 2:
						if (head1 == 3) compare1 = tail1;
						if (head2 == 3) compare2 = tail2;
						break;

					case 3:
						if (head1 == 3) compare1 = tail1;
						if (head2 == 3) compare2 = tail2;
						break;
					}
					affix_locked = std::max(compare1, compare2);
					save[i] = 0;
					//cout << run << ": Locked " << AFFIX_TYPE[i] << " affix of value " << affix_locked << endl;
					locked = true;
				}
			}
			else if (::count[i] == 2) {
				if (curr[i] >= goals_wafer[i]) {
					switch (i) {
					case 0:
						if (head1 == 0 || head1 == 1 || head1 == 2) compare1 = tail1;
						if (head2 == 0 || head2 == 1 || head2 == 2) compare2 = tail2;
						break;
					case 1:
						if (head1 == 3) compare1 = tail1;
						if (head2 == 3) compare2 = tail2;
						break;

					case 2:
						if (head1 == 3) compare1 = tail1;
						if (head2 == 3) compare2 = tail2;
						break;

					case 3:
						if (head1 == 3) compare1 = tail1;
						if (head2 == 3) compare2 = tail2;
						break;
					}
					affix_locked = std::max(compare1, compare2);
					save[i] -= affix_locked;
					//cout << run << ": Locked " << AFFIX_TYPE[i] << " affix of value " << affix_locked << endl;
					locked = true;
					affix_locked += (i + 2) * 100;
				}
			}
		}	
	}
	ncc_total +=  rep * affix_reg;
	double lock_head = (int) (affix_locked / 100);
	double lock_tail = affix_locked - lock_head * 100;
	rep = 0;
	while (!done) {
		++rep;
		double unlock = get_affix();
		double unlock_head = (int)(unlock / 100);
		double unlock_tail = unlock - unlock_head * 100;

		sp_check(affix_locked, unlock);
		for (int i = 0; i < 4; i++) {
			if (head_check(unlock_head, i) && unlock_tail >= save[i]) {
				ncc_total += rep * affix_waf;
				wafer_total += rep * wafer;
				return;
			}
		}
	}
}
int main() {
	// simulates a "truer" random with time-based seeding
	srand(time(NULL));
	fill_data();

	// Affix selection
	cout << "Select desired 1st stat" << endl;
	select_desired(prompt1);

	// You can't have 2 SP affixes of the same type, so we remove the corresponding option if the desired first affix is a SP affix.
	if (prompt1 >= 2) {
		--PROMPT_MAX;
		if (prompt1 == 2) AFFIX_OPEN[2] = 3;
	}

	cout << "Select desired 2nd stat" << endl;
	select_desired(prompt2);

	++::count[prompt1];
	++::count[prompt2];

	// Print selected affix types
	cout << "The selected affix types are " << AFFIX_TYPE[prompt1] << " and " << AFFIX_TYPE[prompt2] << endl;

	set_goals();
	select_wafer();

	if (!WAFER_ON) {
		while (run++ < iter) sim_reg();
	}
	else {
		check_wafer();
		while (run++ < iter) sim_wafer();
	}

	// Output
	cout << fixed << "Total number of Normal Crystal Cores used over " << iter << " runs: " << ncc_total << endl;
	cout << fixed << "Total number of Wafer Stabilizers used over " << iter << " runs: " << wafer_total << endl;
	cout << fixed << "Average number of Normal Crystal Cores used over " << iter << " runs: " << ncc_total / iter << endl;
	cout << fixed << "Average number of Wafer Stabilizers used over " << iter << " runs: " << wafer_total / iter << endl;
}
