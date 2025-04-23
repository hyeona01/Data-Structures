#include "rbtree.h"

#include <stdlib.h>
#include <stdio.h>

rbtree *new_rbtree(void)
{
  rbtree *p = (rbtree *)calloc(1, sizeof(rbtree));

  // nil 노드 할당
  node_t *nil = (node_t *)malloc(sizeof(node_t));

  if (nil == NULL) // 할당 실패
  {
    free(p);
    return NULL;
  }

  // nil 초기화
  nil->color = RBTREE_BLACK;
  nil->left = nil->right = nil->parent = nil;
  p->nil = nil;

  // root 초기화
  p->root = nil;

  return p;
}

void delete_nodes(rbtree *t, node_t *n)
{
  // 후위순회로 삭제
  if (n == t->nil)
    return;
  delete_nodes(t, n->left);
  delete_nodes(t, n->right);
  free(n); // 루트까지
}

void delete_rbtree(rbtree *t)
{
  // 실제 노드 free
  delete_nodes(t, t->root);

  // 구조체 free
  free(t->nil);
  free(t);
}

// ------ Rotation function ------
void left_rotate(rbtree *T, node_t *x)
{
  node_t *y = x->right;

  // 1. 관련 자식 노드 재연결
  if (y->left != T->nil)
  {
    x->right = y->left;
    y->left->parent = x;
  }
  else
  {
    x->right = T->nil;
  }

  // 2. 부모 노드 재연결
  y->parent = x->parent;
  if (x->parent == T->nil) // root 일 때
    T->root = y;
  else if (x->parent->left == x) // left child 일 때
    x->parent->left = y;
  else // right child 일 때
    x->parent->right = y;

  // 3. 당사자들 재연결
  y->left = x;
  x->parent = y;
}

void right_rotate(rbtree *T, node_t *x)
{
  node_t *y = x->left;

  // 1. 관련 자식 노드 재연결
  if (y->right != T->nil)
  {
    x->left = y->right;
    y->right->parent = x;
  }
  else
  {
    x->left = T->nil;
  }

  // 2. 부모 노드 재연결
  y->parent = x->parent;
  if (x->parent == T->nil) // x가 루트일 때
    T->root = y;
  else if (x->parent->right == x) // x가 right child일 때
    x->parent->right = y;
  else
    x->parent->left = y; // x가 left child일 때

  // 3. 당사자들 재연결
  y->right = x;
  x->parent = y;
}

// ------ insert fixup function ------
void insert_fixup(rbtree *t, node_t *x)
{
  node_t *u; // 삼촌 노드
  node_t *p; // 부모 노드

  while (x->parent->color == RBTREE_RED)
  {
    p = x->parent;
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
          x = p; // 펴주고 새로운 x를 기준으로 case 6을 검사
          left_rotate(t, x);
          p = x->parent; // case 6으로 이어지는 과정에서 p를 재사용
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
          x = p; // 펴주고 새로운 x를 기준으로 case 6을 검사
          right_rotate(t, x);
          p = x->parent; // case 6으로 이어지는 과정에서 p를 재사용
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

  return newNode;
}

node_t *rbtree_find(const rbtree *t, const key_t key)
{
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
  node_t *minNode = t->root;
  while (minNode->left != t->nil)
  {
    minNode = minNode->left;
  }

  return minNode;
}

node_t *rbtree_max(const rbtree *t)
{
  node_t *maxNode = t->root;
  while (maxNode->right != t->nil)
  {
    maxNode = maxNode->right;
  }

  return maxNode;
}

// ------ Transplant function(u <- v) ------
void transplant(rbtree *t, node_t *u, node_t *v)
{
  if (t->root == u) // root일 때
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

      // case 1
      // : rotate로 인해 새로운 형제 노드가 black이 되어, case 2~4를 확인해준다.
      if (y->color == RBTREE_RED)
      {
        y->color = RBTREE_BLACK;
        x->parent->color = RBTREE_RED;
        left_rotate(t, x->parent);
        y = x->parent->right;
      }

      // case 2
      // : 형제 노드가 black + 형제 노드의 두 자녀가 black
      if (y->right->color == RBTREE_BLACK && y->left->color == RBTREE_BLACK)
      {
        y->color = RBTREE_RED;
        x = x->parent; // 부모 노드로 extra black 위임(red-and-black or doubly black 처리)
      }
      else
      {
        // case 3
        // : 형제 노드의 자녀 중 하나가 red이고, 그 연결 형태가 꺾인 형태인 경우
        if (y->left->color == RBTREE_RED)
        {
          y->left->color = RBTREE_BLACK;
          y->color = RBTREE_RED;
          right_rotate(t, y);
          y = x->parent->right; // rotate 이후에 형제 노드 업데이트
        }
        // case 4
        // : 형제 노드의 자녀 중 하나가 red이고, 그 연결 형태가 일자 형태인 경우
        y->color = x->parent->color;
        x->parent->color = RBTREE_BLACK;
        y->right->color = RBTREE_BLACK;
        left_rotate(t, x->parent);
        x = t->root;
      }
    }

    // 좌우 반전
    else // x가 right child
    {
      y = x->parent->left;

      // case 1
      // : rotate로 인해 새로운 형제 노드가 black이 되어, case 2~4를 확인해준다.
      if (y->color == RBTREE_RED)
      {
        y->color = RBTREE_BLACK;
        x->parent->color = RBTREE_RED;
        right_rotate(t, x->parent);
        y = x->parent->left;
      }

      // case 2
      // 형제 노드가 black + 자식 모두 black
      if (y->right->color == RBTREE_BLACK && y->left->color == RBTREE_BLACK)
      {
        y->color = RBTREE_RED;
        x = x->parent; // 부모 노드로 extra black 위임(red-and-black or doubly black 처리)
      }
      else
      {
        // case 3
        // : 형제 노드의 자녀 중 하나가 red이고, 그 연결 형태가 꺾인 형태인 경우
        if (y->right->color == RBTREE_RED)
        {
          y->right->color = RBTREE_BLACK;
          y->color = RBTREE_RED;
          left_rotate(t, y);
          y = x->parent->left; // rotate로 변경된 형제 노드를 업데이트 해줌
        }
        // case 4
        // : 형제 노드의 자녀 중 하나가 red이고, 그 연결 형태가 일자 형태인 경우
        y->color = x->parent->color;
        x->parent->color = RBTREE_BLACK;
        y->left->color = RBTREE_BLACK;
        right_rotate(t, x->parent);
        x = t->root;
      }
    }
  }
  // 루트 조건으로 while문을 탈출했을 때, 루트 색을 black으로 바꿔주기 위함이다.
  x->color = RBTREE_BLACK;
}

int rbtree_erase(rbtree *t, node_t *p)
{
  node_t *x;     // 삭제되는 노드
  node_t *y = p; // successor를 고려하는 경우, 대체되는 노드
  color_t original_color = y->color;

  // 오른쪽 자식이 있는 경우(실제 자식이 없고 nil 노드만 있는 경우도 포함 → p를 삭제하기 때문에 대체되는 노드 업데이트 필수)
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
    {
      // 대체 노드(successor)의 오른쪽 자식이 successor의 자리에 위치되도록
      transplant(t, y, y->right); // y.right -> y 이동
      // 대체 노드가 기존 노드와의 관계를 이어받도록
      y->right = p->right;
      y->right->parent = y;
    }
    else
    {
      x->parent = y; // extra black 처리를 해줄 때, 부모 + 형제를 찾아갈 수 있도록 함
    }
    transplant(t, p, y); // y -> p 삭제 노드의 위치에 대체 노드 이동
    y->left = p->left;
    y->left->parent = y;
    y->color = p->color;
  }

  if (original_color == RBTREE_BLACK)
  {
    delete_fixup(t, x); // 삭제된 노드의 대체 노드가 extra black을 가짐
  }

  free(p);
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
  // 중위순회로 arr에 담기
  int idx = 0;
  inorder(t->root, t, arr, &idx);
  return 0;
}
