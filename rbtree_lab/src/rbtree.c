#include "rbtree.h"

#include <stdlib.h>
#include <stdio.h>

// ========== debug ==========
void print_rb_tree(const rbtree *t, node_t *nowNode)
{
  if (nowNode == t->nil)
  {
    return;
  }
  else
  {
    const char *color_names[] = {"RED", "BLACK"};
    printf("my: %d ", nowNode->key);
    printf("color: %s ", color_names[nowNode->color]);
    if (nowNode->left != NULL)
    {
      printf("left: %d ", nowNode->left->key);
    }
    if (nowNode->right != NULL)
    {
      printf("right: %d ", nowNode->right->key);
    }
    printf("//");
    print_rb_tree(t, nowNode->left);
    print_rb_tree(t, nowNode->right);
  }
}
// ========== debug ==========

// ------ Rotation function ------
void left_rotate(rbtree *T, node_t *x)
{
  node_t *y = x->right;
  x->right = y->left;

  if (y->left != T->nil)
  {
    y->left->parent = x;
  }
  y->parent = x->parent;

  if (x->parent == T->nil)
    T->root = y;
  else if (x->parent->left == x)
    x->parent->left = y;
  else
    x->parent->right = y;

  y->left = x;
  x->parent = y;
}

void right_rotate(rbtree *T, node_t *x)
{
  node_t *y = x->left;
  x->left = y->right;

  if (y->right != T->nil)
  {
    y->right->parent = x;
  }
  y->parent = x->parent;

  if (x->parent == T->nil)
    T->root = y;
  else if (x->parent->left == x)
    x->parent->left = y;
  else
    x->parent->right = y;

  y->right = x;
  x->parent = y;
}

// ------ insert fixup function ------
void insert_fixup(rbtree *t, node_t *x)
{
  while (x->parent->color == RBTREE_RED)
  {
    node_t *p = x->parent;
    node_t *u;

    if (p == p->parent->left) // 부모가 left child
    {
      u = p->parent->right;       // 삼촌 노드
      if (u->color == RBTREE_RED) // case 4 : 부모, 삼촌이 red
      {
        p->color = RBTREE_BLACK;
        u->color = RBTREE_BLACK;
        p->parent->color = RBTREE_RED;
        x = p->parent; // --- 추가 검사 ---
      }
      else // 부모 red + 삼촌 black
      {
        if (p->right == x) // case 5: 꺾인 형태
        {
          x = p; // node update
          left_rotate(t, x);
        }

        // case 6 : 일자 형태
        p->color = RBTREE_BLACK;
        p->parent->color = RBTREE_RED;

        right_rotate(t, p->parent); // 조부모를 기준으로 회전
      }
    }
    // 좌우 반전
    else // 부모가 right child
    {
      u = p->parent->left;        // 삼촌 노드
      if (u->color == RBTREE_RED) // case 4 : 부모, 삼촌이 red
      {
        p->color = RBTREE_BLACK;
        u->color = RBTREE_BLACK;
        p->parent->color = RBTREE_RED;
        x = p->parent; // --- 추가 검사 ---
      }
      else // 부모 red + 삼촌 black
      {
        if (p->left == x) // case 5: 꺾인 형태
        {
          x = p; // node update
          right_rotate(t, x);
        }

        // case 6 : 일자 형태
        p->color = RBTREE_BLACK;
        p->parent->color = RBTREE_RED;

        left_rotate(t, p->parent); // 조부모를 기준으로 회전
      }
    }
  }
  t->root->color = RBTREE_BLACK; // case 1 루트 조건으로 while문을 탈출했을 때
}

// ------ Transplant function(u <- v) ------
void transplant(rbtree *t, node_t *u, node_t *v)
{
  if (u->parent == t->nil) // root일 때
                           // if (t->root == u) // root일 때
    t->root = v;
  else if (u == u->parent->left)
    u->parent->left = v;
  else
    u->parent->right = v;
  v->parent = u->parent;
}

// ------ Find minimum function ------
node_t *find_min(rbtree *t, node_t *x)
{
  while (x->left != t->nil)
  {
    x = x->left;
  }

  return x;
}

// ------ Delete fixup function ------
void delete_fixup(rbtree *t, node_t *x)
{
  while (x != t->root && x->color == RBTREE_BLACK)
  {
    node_t *y; // 형제 노드

    if (x == x->parent->left) // x가 left child
    {
      y = x->parent->right;
      if (y->color == RBTREE_RED) // case 1
      {
        y->color = RBTREE_BLACK;
        x->parent->color = RBTREE_RED;
        left_rotate(t, x->parent);
        y = x->parent->right; // why?????????????
      }
      // why????????????? 왜 else if나, else가 아니고?
      if (y->right->color == RBTREE_BLACK && y->left->color == RBTREE_BLACK) // case 2
      {
        y->color = RBTREE_RED;
        // 속성에 위반되는 지(부모 노드가 red) 확인해야 함
        x = x->parent;
      }
      else
      {
        if (y->right->color == RBTREE_BLACK) // case 3
        {
          y->left->color = RBTREE_BLACK;
          y->color = RBTREE_RED;
          right_rotate(t, y);
          y = x->parent->right; // why?????????TT
        }
        y->color = x->parent->color; // case 4
        x->parent->color = RBTREE_BLACK;
        y->right->color = RBTREE_BLACK;
        left_rotate(t, x->parent);
        // break; // 완료
        x = t->root;
      }
    }

    // 좌우 반전
    else // x가 right child
    {
      y = x->parent->left;
      if (y->color == RBTREE_RED) // case 1
      {
        y->color = RBTREE_BLACK;
        x->parent->color = RBTREE_RED;
        right_rotate(t, x->parent);
        y = x->parent->left;
      }
      // else
      // {
      if (y->right->color == RBTREE_BLACK && y->left->color == RBTREE_BLACK) // case 2
      {
        y->color = RBTREE_RED;
        // 속성에 위반되는 지(부모 노드가 red) 확인해야 함
        x = x->parent;
      }
      else
      {
        if (y->left->color == RBTREE_BLACK) // case 3
        {
          y->right->color = RBTREE_BLACK;
          y->color = RBTREE_RED;
          left_rotate(t, y);
          y = x->parent->left; // why???????
        }
        y->color = x->parent->color; // case 4
        x->parent->color = RBTREE_BLACK;
        y->left->color = RBTREE_BLACK;
        right_rotate(t, x->parent);
        // break; // 완료
        x = t->root;
      }
    }
  }
  // 루트로 while문을 탈출했을 때, 루트 색을 black으로 바꿔준다.
  x->color = RBTREE_BLACK;
}

rbtree *new_rbtree(void)
{
  rbtree *p = (rbtree *)calloc(1, sizeof(rbtree));
  // TODO: initialize struct if needed
  p->nil = (node_t *)malloc(sizeof(node_t));
  p->nil->color = RBTREE_BLACK;
  p->nil->left = p->nil->right = p->nil->parent = p->nil;
  p->root = p->nil;

  return p;
}

void delete_nodes(rbtree *t, node_t *n)
{
  if (n == t->nil)
    return;
  delete_nodes(t, n->left);
  delete_nodes(t, n->right);
  free(n);
}

void delete_rbtree(rbtree *t)
{
  // TODO: reclaim the tree nodes's memory
  // 실제 노드 free
  delete_nodes(t, t->root);

  // 구조체 free
  free(t->nil);
  free(t);
}

node_t *rbtree_insert(rbtree *t, const key_t key)
{
  // TODO: implement insert
  node_t *x = t->root;
  node_t *p = t->nil;

  // 삽입 노드 초기화
  node_t *newNode = (node_t *)malloc(sizeof(node_t));
  newNode->color = RBTREE_RED;
  newNode->left = t->nil;
  newNode->right = t->nil;
  newNode->key = key;

  // 1. 삽입할 위치를 찾아 내려가기
  while (x != t->nil)
  {
    p = x;
    if (key < x->key)
      x = x->left;
    else
      x = x->right;
  }
  newNode->parent = p;

  // 2. 위치에 red color로 삽입하기
  if (p == t->nil) // 트리가 비어있을 떄
    t->root = newNode;
  else if (key < p->key) // leaf 보다 작을 때
    p->left = newNode;
  else
    p->right = newNode; // leaf 보다 클 때

  insert_fixup(t, newNode);

  // printf("추가된 값: %d \n", newNode->key);
  // printf("\n-------------\n");
  // print_rb_tree(t, t->root);
  return t->root;
}

node_t *rbtree_find(const rbtree *t, const key_t key)
{
  // TODO: implement find
  node_t *x = t->root;

  while (x != t->nil)
  {
    if (key == x->key)
      return x; // 찾음
    if (x->key > key)
      x = x->left;
    else
      x = x->right;
  }
  return NULL; // 찾는 값이 없음
}

node_t *rbtree_min(const rbtree *t)
{
  // TODO: implement find
  node_t *minNode = t->root;
  while (minNode->left != t->nil)
  {
    minNode = minNode->left;
  }

  return minNode;
}

node_t *rbtree_max(const rbtree *t)
{
  // TODO: implement find
  node_t *maxNode = t->root;
  while (maxNode->right != t->nil)
  {
    maxNode = maxNode->right;
  }

  return maxNode;
}

int rbtree_erase(rbtree *t, node_t *p)
{
  // TODO: implement erase
  node_t *x;     // 삭제하는 노드
  node_t *y = p; // 대체되는 노드
  color_t original_color = y->color;

  // 오른쪽 자식이 있는 경우
  if (p->left == t->nil)
  {
    x = p->right;
    transplant(t, p, p->right);
  }
  // 왼쪽 자식만 있는 경우
  else if (p->right == t->nil)
  {
    x = p->left;
    transplant(t, p, p->left);
  }

  // 자식이 두개 이상
  else
  {
    y = find_min(t, p->right);
    original_color = y->color; // 실제로 삭제되는 색

    x = y->right; // 대체되는 노드의 자식에 extra black 부여

    if (y != p->right) // 바로 아래 노드가 아니라면
    // if (p != y->parent) // 바로 아래 노드가 아니라면
    {
      // 대체 노드(successor)의 오른쪽 자식이 successor의 자리에 위치되도록
      transplant(t, y, y->right);
      // 대체 노드가 기존 노드와의 관계를 이어받도록
      y->right = p->right;
      y->right->parent = y;
    }
    else
      x->parent = y;
    transplant(t, p, y); // 삭제 노드의 위치에 대체 노드 이동
    y->left = p->left;
    y->left->parent = y;
    y->color = p->color;
  }

  if (original_color == RBTREE_BLACK)
    delete_fixup(t, x); // 삭제된 노드의 대체 노드가 extra black을 가짐

  // free
  free(p);
  // if (y != p)
  //   free(p);
  // else
  //   free(y);

  return 0;
}

void inorder(node_t *x, const rbtree *t, key_t *arr, int *idx) // 왼 - 루 - 오
{
  if (x == t->nil)
    return;

  inorder(x->left, t, arr, idx);
  arr[(*idx)++] = x->key;
  inorder(x->right, t, arr, idx);
}

int rbtree_to_array(const rbtree *t, key_t *arr, const size_t n)
{
  // TODO: implement to_array
  // 중위순회로 arr에 담기
  int idx = 0;
  inorder(t->root, t, arr, &idx);
  return 0;
}
