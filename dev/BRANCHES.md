# BRANCHES CONVENTION

### Why We Use Branches

Branches are a fundamental part of Git that allow us to work on different features, bug fixes, or experiments in isolation from the main codebase. By using branches, we can:
- Develop new features without affecting the stable version of the code.
- Fix bugs without introducing new issues to the main branch.
- Collaborate with other developers without conflicts.
- Test new ideas without risking the integrity of the main codebase.

### Branching Strategy

We follow a branching strategy organized by domain and development stage to keep work structured and maintain a stable production flow:

* **main**: The main branch that always contains stable, production-ready code.
* **dev**: The integration branch where all completed work is merged for testing and validation before release to `main`.
* **server**: The base branch for all backend-related development. Features, bug fixes, and improvements related to the server are created from this branch before being merged into `dev`.
* **GUI**: The base branch for all frontend/user interface development. All UI-related work branches off from here and is later integrated into `dev`.
* **IA**: The base branch for all AI-related development (models, logic, integrations). All AI-related feature and fix branches originate here before merging into `dev`.
* **feature/**: Branches created for developing new features. They are created from the relevant base branch (`server`, `GUI`, or `IA`) and merged back into `dev` once completed and tested.
* **bugfix/**: Branches created for fixing non-critical bugs. They are created from the relevant base branch (`server`, `GUI`, or `IA`) and merged back into `dev` after validation.
* **hotfix/**: Branches created for urgent fixes that must be applied directly to `main`. They are branched from `main` and merged back into both `main` and `dev` after completion to ensure consistency.

This structure ensures that each domain evolves independently while still converging into a unified and tested development flow through `dev` before reaching production in `main`.


### Basic Rules

1. **Use Lowercase Alphanumerics, Hyphens, and Dots**: Always use lowercase letters (a–z), numbers (0–9), and hyphens (-) to separate words. Avoid special characters, underscores, or spaces.
2. **No Consecutive, Leading, or Trailing Hyphens or Dots**: Ensure hyphens and dots are not consecutive (e.g., `feature/new--login`, `bugfix/issue-123-bug-fix`), and do not appear at the start or end of the branch name (e.g., `feature/-new-login`).
3. **Keep It Clear and Concise**: Branch names should be descriptive yet concise, clearly indicating the purpose of the work.
4. **Include issue numbers**: If the branch is related to a specific issue or task, include the issue number in the branch name (e.g., `feature/123-new-login`).

### Benefits of This Strategy
1. **Organized Workflow**: Clear separation of different types of work (features, bug fixes, hotfixes) helps keep the codebase organized and manageable.
2. **Parallel Development**: Multiple developers can work on different branches simultaneously without conflicts, improving productivity.
3. **Stable Main Branch**: By keeping the main branch stable and only merging tested code, we reduce the risk of introducing bugs into production.
4. **Easier Code Reviews**: Smaller, focused branches make code reviews easier and more effective, as reviewers can focus on specific changes related to a feature or bug fix.
5. **Better Collaboration**: Branches allow for better collaboration among team members, as they can work on their own branches and merge changes when ready, without stepping on each other's toes.
6. **Efficient Release Management**: By using branches for features and bug fixes, we can manage releases more efficiently, as we can choose which features and fixes to include in each release by merging specific branches into main.
